#pragma once
using json = nlohmann::json;


// ─────────────────────────────────────────
// データ構造
// ─────────────────────────────────────────
struct Todo {
    int         id;
    std::string title;
    bool        done;
    std::string created_at;
};

// ─────────────────────────────────────────
//  todo helper
// ─────────────────────────────────────────
class MyTodo {
public:
    explicit MyTodo(const std::string& path) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK)
            die("open");
        exec("PRAGMA journal_mode=WAL;");
        exec(R"(
            CREATE TABLE IF NOT EXISTS todos (
                id         INTEGER PRIMARY KEY AUTOINCREMENT,
                title      TEXT    NOT NULL,
                done       INTEGER NOT NULL DEFAULT 0,
                created_at TEXT    NOT NULL
            );
        )");
    }
    ~MyTodo() { sqlite3_close(db_); }

    // ── Write ──────────────────────────────
    void add(const std::string& title) {
        std::string now = timestamp();
        sqlite3_stmt* s;
        prepare("INSERT INTO todos (title, done, created_at) VALUES (?, 0, ?);", &s);
        sqlite3_bind_text(s, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 2, now.c_str(),   -1, SQLITE_TRANSIENT);
        step_and_finalize(s);
        std::cout << "✓ 追加しました: [" << sqlite3_last_insert_rowid(db_) << "] " << title << "\n";
    }

    void update(int id) {
        sqlite3_stmt* s;
        prepare("UPDATE todos SET done = 1 WHERE id = ?;", &s);
        sqlite3_bind_int(s, 1, id);
        step_and_finalize(s);
        if (sqlite3_changes(db_) == 0)
            std::cout << "ID " << id << " が見つかりません。\n";
        else
            std::cout << "✓ 完了しました: ID " << id << "\n";
    }
    
    void reset_complete(int id) {
        sqlite3_stmt* s;
        prepare("UPDATE todos SET done = 0 WHERE id = ?;", &s);
        sqlite3_bind_int(s, 1, id);
        step_and_finalize(s);
        if (sqlite3_changes(db_) == 0)
            std::cout << "ID " << id << " が見つかりません。\n";
        else
            std::cout << "✓ 未完了に戻しました: ID " << id << "\n";
    }

    void remove(int id) {
        sqlite3_stmt* s;
        prepare("DELETE FROM todos WHERE id = ?;", &s);
        sqlite3_bind_int(s, 1, id);
        step_and_finalize(s);
        if (sqlite3_changes(db_) == 0)
            std::cout << "ID " << id << " が見つかりません。\n";
        else
            std::cout << "✓ 削除しました: ID " << id << "\n";
    }

    void clear_done() {
        exec("DELETE FROM todos WHERE done = 1;");
        std::cout << "✓ 完了済みタスクをすべて削除しました。\n";
    }

    // ── Read ───────────────────────────────
    std::vector<Todo> list(const std::string& filter = "all") {
        std::string sql = "SELECT id, title, done, created_at FROM todos";
        if (filter == "pending")  sql += " WHERE done = 0";
        if (filter == "done")     sql += " WHERE done = 1";
        sql += " ORDER BY id DESC;";

        sqlite3_stmt* s;
        prepare(sql, &s);
        std::vector<Todo> rows;
        while (sqlite3_step(s) == SQLITE_ROW) {
            rows.push_back({
                sqlite3_column_int (s, 0),
                reinterpret_cast<const char*>(sqlite3_column_text(s, 1)),
                sqlite3_column_int (s, 2) != 0,
                reinterpret_cast<const char*>(sqlite3_column_text(s, 3))
            });
        }
        sqlite3_finalize(s);
        return rows;
    }
    std::string todo_to_json(const Todo& t) {
        std::ostringstream oss;
        oss << "{"
            << "\"id\":"    << t.id           << ","
            << "\"title\":\"" << t.title      << "\","
            << "\"done\":"  << (t.done ? "true" : "false")
            << "}";
        return oss.str();
    }

    std::string todos_to_json(const std::vector<Todo>& todos) {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < todos.size(); ++i) {
            if (i > 0) oss << ",";
            oss << todo_to_json(todos[i]);
        }
        oss << "]";
        return oss.str();
    }
    /**
    *
    * @param
    *
    * @return
    */
    void todos_list_handler(httplib::Response& res) {
        std::string ret = "";
        try{
            auto todos = list("all");
            auto resp = todos_to_json(todos);
            res.status = 201;
            res.set_content(resp, "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    }
    /**
    *
    * @param
    *
    * @return
    */
    void todos_add_handler(const httplib::Request& req, httplib::Response& res) {
        std::string ret = "";
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            json j = json::parse(req.body);

            std::string title = j.at("title").get<std::string>();
            std::cout << "title=" << title << "\n";

            add(title);
            res.status = 201;
            res.set_content("OK", "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    }
    /**
    *
    * @param
    *
    * @return
    */    
    void todos_update_handler(const httplib::Request& req, httplib::Response& res) {
        std::string ret = "";
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            int id = std::stoi(req.matches[1]);
            std::cout << "id=" << id << "\n";
            update(id);
            res.status = 201;
            res.set_content("OK", "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    }    
    /**
    *
    * @param
    *
    * @return
    */
    void todos_delete_handler(const httplib::Request& req, httplib::Response& res) {
        std::string ret = "";
        int id = std::stoi(req.matches[1]);
        try{
            remove(id);
            res.status = 200;
            res.set_content("OK", "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }
    }

private:
    sqlite3* db_ = nullptr;

    void exec(const std::string& sql) {
        char* err = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err ? err : "unknown";
            sqlite3_free(err);
            die(msg);
        }
    }

    void prepare(const std::string& sql, sqlite3_stmt** s) {
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, s, nullptr) != SQLITE_OK)
            die(sqlite3_errmsg(db_));
    }

    void step_and_finalize(sqlite3_stmt* s) {
        sqlite3_step(s);
        sqlite3_finalize(s);
    }

    static std::string timestamp() {
        std::time_t t = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        return buf;
    }

    [[noreturn]] static void die(const std::string& msg) {
        std::cerr << "DB error: " << msg << "\n";
        std::exit(1);
    }
};
