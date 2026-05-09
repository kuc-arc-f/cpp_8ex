#include "httplib.h"
#include <nlohmann/json.hpp>
#include <libpq-fe.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

// ──────────────────────────────────────────────
// DB helper: RAII wrapper around PGconn
// ──────────────────────────────────────────────
class DB {
public:
    explicit DB(const std::string& conninfo) {
        conn_ = PQconnectdb(conninfo.c_str());
        if (PQstatus(conn_) != CONNECTION_OK) {
            std::string err = PQerrorMessage(conn_);
            PQfinish(conn_);
            throw std::runtime_error("DB connection failed: " + err);
        }
    }
    ~DB() { PQfinish(conn_); }

    // Non-copyable
    DB(const DB&) = delete;
    DB& operator=(const DB&) = delete;

    PGconn* get() const { return conn_; }

private:
    PGconn* conn_ = nullptr;
};

// ──────────────────────────────────────────────
// DB helper: RAII wrapper around PGresult
// ──────────────────────────────────────────────
class PGResult {
public:
    explicit PGResult(PGresult* r) : res_(r) {}
    ~PGResult() { if (res_) PQclear(res_); }

    PGResult(const PGResult&) = delete;
    PGResult& operator=(const PGResult&) = delete;

    PGresult* get() const { return res_; }
    ExecStatusType status() const { return PQresultStatus(res_); }
    bool ok()  const { return status() == PGRES_TUPLES_OK || status() == PGRES_COMMAND_OK; }
    std::string error() const { return PQresultErrorMessage(res_); }

    // Convenience: return field value (empty string if NULL)
    std::string val(int row, int col) const {
        if (PQgetisnull(res_, row, col)) return "";
        return PQgetvalue(res_, row, col);
    }
    int rows() const { return PQntuples(res_); }

private:
    PGresult* res_;
};

// ──────────────────────────────────────────────
// Utility: escape a single-quoted value for SQL
// (prefer parameterized queries where possible)
// ──────────────────────────────────────────────
static std::string escape(PGconn* conn, const std::string& s) {
    std::vector<char> buf(s.size() * 2 + 1);
    PQescapeStringConn(conn, buf.data(), s.c_str(), s.size(), nullptr);
    return buf.data();
}

// ──────────────────────────────────────────────
// Convert a single DB row to JSON object
// Column order must match SELECT order below
// ──────────────────────────────────────────────
static json rowToJson(const PGResult& r, int row) {
    json obj;
    obj["id"]          = std::stoi(r.val(row, 0));
    obj["title"]       = r.val(row, 1);
    obj["description"] = r.val(row, 2);
    obj["due_date"]    = r.val(row, 3);
    obj["priority"]    = r.val(row, 4).empty() ? 0 : std::stoi(r.val(row, 4));
    obj["done"]        = (r.val(row, 5) == "t");
    obj["created_at"]  = r.val(row, 6);
    obj["updated_at"]  = r.val(row, 7);
    return obj;
}

// Column list used in all SELECT queries
static const char* SELECT_COLS =
    "id, title, description, due_date, priority, done, created_at, updated_at";

// ──────────────────────────────────────────────
// Response helpers
// ──────────────────────────────────────────────
static void sendJson(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

static void sendError(httplib::Response& res, int status, const std::string& msg) {
    sendJson(res, status, {{"error", msg}});
}

// ──────────────────────────────────────────────
// Main
// ──────────────────────────────────────────────
int main() {
    // ── Connection string from environment (or hard-coded default) ──
    const char* env_conn = std::getenv("DATABASE_URL");
    std::string conninfo = env_conn
        ? env_conn
        : "host=localhost port=5432 dbname=todo_db user=postgres password=postgres";

    std::unique_ptr<DB> db;
    try {
        db = std::make_unique<DB>(conninfo);
        std::cout << "[DB] Connected to PostgreSQL\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    httplib::Server svr;

    // ── CORS (for browser clients) ──────────────────────────────────
    svr.set_default_headers({
        {"Access-Control-Allow-Origin",  "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // ──────────────────────────────────────────
    // GET /todos   - list (optional ?done=true/false)
    // ──────────────────────────────────────────
    svr.Get("/todos", [&](const httplib::Request& req, httplib::Response& res) {
        std::string sql = std::string("SELECT ") + SELECT_COLS +
                          " FROM todos";

        if (req.has_param("done")) {
            std::string v = req.get_param_value("done");
            if (v == "true" || v == "1")
                sql += " WHERE done = TRUE";
            else if (v == "false" || v == "0")
                sql += " WHERE done = FALSE";
        }
        sql += " ORDER BY id DESC";

        PGResult r(PQexec(db->get(), sql.c_str()));
        if (!r.ok()) {
            return sendError(res, 500, r.error());
        }

        json arr = json::array();
        for (int i = 0; i < r.rows(); ++i)
            arr.push_back(rowToJson(r, i));

        sendJson(res, 200, arr);
    });

    // ──────────────────────────────────────────
    // POST /todos   - create
    // ──────────────────────────────────────────
    svr.Post("/todos", [&](const httplib::Request& req, httplib::Response& res) {
        json body;
        try { body = json::parse(req.body); }
        catch (...) { return sendError(res, 400, "Invalid JSON"); }

        if (!body.contains("title") || body["title"].get<std::string>().empty())
            return sendError(res, 400, "title is required");

        std::string title       = escape(db->get(), body["title"].get<std::string>());
        std::string description = body.value("description", "");
        description = escape(db->get(), description);

        std::string due_date_sql = "NULL";
        if (body.contains("due_date") && !body["due_date"].get<std::string>().empty())
            due_date_sql = "'" + escape(db->get(), body["due_date"].get<std::string>()) + "'";

        int priority = body.value("priority", 0);
        bool done    = body.value("done", false);

        std::string sql =
            "INSERT INTO todos (title, description, due_date, priority, done) VALUES ("
            "'" + title + "', "
            "'" + description + "', "
            + due_date_sql + ", "
            + std::to_string(priority) + ", "
            + (done ? "TRUE" : "FALSE") +
            ") RETURNING " + SELECT_COLS;

        PGResult r(PQexec(db->get(), sql.c_str()));
        if (r.status() != PGRES_TUPLES_OK)
            return sendError(res, 500, r.error());

        sendJson(res, 201, rowToJson(r, 0));
    });

    // ──────────────────────────────────────────
    // GET /todos/:id
    // ──────────────────────────────────────────
    svr.Get(R"(/todos/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        std::string sql = std::string("SELECT ") + SELECT_COLS +
                          " FROM todos WHERE id = " + id;

        PGResult r(PQexec(db->get(), sql.c_str()));
        if (!r.ok())          return sendError(res, 500, r.error());
        if (r.rows() == 0)    return sendError(res, 404, "Not found");

        sendJson(res, 200, rowToJson(r, 0));
    });

    // ──────────────────────────────────────────
    // PUT /todos/:id   - update (partial OK)
    // ──────────────────────────────────────────
    svr.Put(R"(/todos/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];

        json body;
        try { body = json::parse(req.body); }
        catch (...) { return sendError(res, 400, "Invalid JSON"); }

        std::vector<std::string> sets;

        if (body.contains("title"))
            sets.push_back("title = '" + escape(db->get(), body["title"].get<std::string>()) + "'");
        if (body.contains("description"))
            sets.push_back("description = '" + escape(db->get(), body["description"].get<std::string>()) + "'");
        if (body.contains("due_date")) {
            auto v = body["due_date"].get<std::string>();
            sets.push_back("due_date = " + (v.empty() ? "NULL" : "'" + escape(db->get(), v) + "'"));
        }
        if (body.contains("priority"))
            sets.push_back("priority = " + std::to_string(body["priority"].get<int>()));
        if (body.contains("done"))
            sets.push_back(std::string("done = ") + (body["done"].get<bool>() ? "TRUE" : "FALSE"));

        if (sets.empty())
            return sendError(res, 400, "No fields to update");

        std::string joined;
        for (size_t i = 0; i < sets.size(); ++i)
            joined += (i ? ", " : "") + sets[i];

        std::string sql =
            "UPDATE todos SET " + joined +
            " WHERE id = " + id +
            " RETURNING " + SELECT_COLS;

        PGResult r(PQexec(db->get(), sql.c_str()));
        if (!r.ok())         return sendError(res, 500, r.error());
        if (r.rows() == 0)   return sendError(res, 404, "Not found");

        sendJson(res, 200, rowToJson(r, 0));
    });

    // ──────────────────────────────────────────
    // DELETE /todos/:id
    // ──────────────────────────────────────────
    svr.Delete(R"(/todos/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        std::string sql = "DELETE FROM todos WHERE id = " + id + " RETURNING id";

        PGResult r(PQexec(db->get(), sql.c_str()));
        if (!r.ok())        return sendError(res, 500, r.error());
        if (r.rows() == 0)  return sendError(res, 404, "Not found");

        sendJson(res, 200, {{"message", "Deleted"}, {"id", std::stoi(id)}});
    });

    // ── Start server ────────────────────────────────────────────────
    const int PORT = 8000;
    std::cout << "[Server] Listening on http://0.0.0.0:" << PORT << "\n";
    svr.listen("0.0.0.0", PORT);

    return 0;
}
