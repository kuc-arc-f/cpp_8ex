#include "httplib.h"
#include <cstdlib>
#include <dotenv.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <libpq-fe.h>

#include <stdexcept>
#include <string>
#include "my_db.hpp"

using json = nlohmann::json;

const std::string COOKIE_KEY_USER = "http_11user";

std::map<std::string, std::string> parse_cookies(const std::string& cookie_str) {
    std::map<std::string, std::string> cookies;
    std::istringstream iss(cookie_str);
    std::string token;

    while (std::getline(iss, token, ';')) {
        size_t pos = token.find('=');
        if (pos != std::string::npos) {
            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);

            // 先頭・末尾の空白やダブルクォートを削除
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t\""));
            value.erase(value.find_last_not_of(" \t\"") + 1);

            cookies[key] = value;
        }
    }
    return cookies;
}
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
    obj["done"]        = (r.val(row, 2) == "t");
    obj["created_at"]  = r.val(row, 3);
    obj["updated_at"]  = r.val(row,4);
    return obj;
}

// Column list used in all SELECT queries
static const char* SELECT_COLS =
    "id, title, done, created_at, updated_at";

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


// ファイルを文字列として読み込むユーティリティ
std::string readFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return "";
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
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
    dotenv::init();
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

        bool done    = body.value("done", false);

        std::string sql =
            "INSERT INTO todos (title, done) VALUES ("
            "'" + title + "', "
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
    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        std::string ret = "";
        const char* env_name = std::getenv("USER_NAME");
        if (env_name) {
            std::cout << "env_name=" << env_name << std::endl;
        }    
        const char* env_password = std::getenv("PASSWORD");
        if (env_password) {
            std::cout << "env_password=" << env_password << std::endl;
        }

        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            json j = json::parse(req.body);

            //username
            std::string username = j.at("email").get<std::string>();
            std::cout << "username=" << username << "\n";
            //password
            std::string password = j.at("password").get<std::string>();
            std::cout << "password=" << password << "\n";
            if(env_name == username && env_password == password){
                res.status = 200;
                res.set_content("OK", "application/json");
                return;
            }
            res.status = 400;
            res.set_content("NG", "application/json");
            return;
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }
    });    
    // ─── SSR HTMLページ (CSSを<link>タグで参照) ───
    svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        try{
            const char* valid_login = std::getenv("VALID_LOGIN");
            std::cout << "#start get /" << std::endl;
            std::cout << "valid_login=" << valid_login << std::endl;
            if (valid_login && valid_login == std::string("true")) {
                // Cookie ヘッダーを取得
                auto cookies = req.get_header_value("Cookie");
                if (!cookies.empty()) {
                    std::cout << "Received Cookies: " << cookies << std::endl;
                    // 必要に応じてパース（例： "sessionid=abc123; user=john"）
                    // 簡単なパーサーを使って特定のキーの値を取得することも可能
                    auto cookie_values = parse_cookies(cookies);

                    if (cookie_values.count(COOKIE_KEY_USER)) {
                        std::cout << "value: " << cookie_values[COOKIE_KEY_USER] << std::endl;
                    }else{
                        //none-cookie
                        std::cout << "#none cookie: " << std::endl;
                        res.set_redirect("/login");
                        return;
                    }
                } else {
                    std::cout << "No cookies received." << std::endl;
                    res.set_redirect("/login");
                    return;
                }
            }    
            res.set_file_content("./html/root.html");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    });
    svr.Get("/about", [](const httplib::Request& req, httplib::Response& res) {
        res.set_file_content("./html/root.html");
    });
    svr.Get("/login", [](const httplib::Request& req, httplib::Response& res) {
        res.set_file_content("./html/root.html");
    });        

    // ─── CSSファイルの配信 ───
    svr.Get("/style.css", [](const httplib::Request&, httplib::Response& res) {
        std::string css = readFile("static/style.css");
        if (css.empty()) {
            res.status = 404;
            res.set_content("CSS not found", "text/plain");
            return;
        }
        res.set_content(css, "text/css; charset=utf-8");
    });
    svr.Get("/client.js", [](const httplib::Request&, httplib::Response& res) {
        std::string css = readFile("static/client.js");
        if (css.empty()) {
            res.status = 404;
            res.set_content("CSS not found", "text/plain");
            return;
        }
        res.set_content(css, "application/javascript");
    });     

    // ── Start server ────────────────────────────────────────────────
    const int PORT = 8000;
    std::cout << "#start-server"  << std::endl;
    std::cout << "[Server] Listening on http://0.0.0.0:" << PORT << "\n";
    svr.listen("0.0.0.0", PORT);

    return 0;
}
