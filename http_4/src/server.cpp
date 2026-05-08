#include "httplib.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <mutex>
#include <nlohmann/json.hpp> // JSONライブラリ
#include <vector>
#include <string>
#include <sqlite3.h>
#include <sstream>

#include "my_todo.hpp"

using json = nlohmann::json;

std::string DB_PATH = "./todo.db";

// インメモリストレージ
static std::vector<Todo> g_todos;
static std::mutex        g_mutex;

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

// ─────────────────────────────────────────
// ヘルパー：JSON から値を取り出す（簡易版）
// ─────────────────────────────────────────
std::string extract_string(const std::string& json, const std::string& key) {
    // "key":"value" を探す
    std::string pattern = "\"" + key + "\":\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return "";
    pos += pattern.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

bool extract_bool(const std::string& json, const std::string& key, bool def = false) {
    std::string pattern = "\"" + key + "\":";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return def;
    pos += pattern.size();
    return json.substr(pos, 4) == "true";
}

// ─────────────────────────────────────────
// CORS ヘッダー（ブラウザからのアクセス用）
// ─────────────────────────────────────────
void set_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ─────────────────────────────────────────
// main
// ─────────────────────────────────────────
int main() {
    httplib::Server svr;

    std::cout << "db_path=" << DB_PATH << " \n";

    // ── OPTIONS（プリフライト） ──────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        set_cors(res);
        res.status = 204;
    });
    // ─── SSR HTMLページ (CSSを<link>タグで参照) ───
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::string html = R"(<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <title>cpp-httplib SSR</title>
    <!-- CSSはサーバーから /style.css として配信 -->
    <script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4"></script>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
<!-- 
    <h1>Hello from , cpp-httplib SSR!</h1>
    <p>このページはサーバーサイドでレンダリングされています。</p>
-->
    <div id="app"></div>
    <script type="module" src="/client.js"></script>
</body>
</html>)";
        res.set_content(html, "text/html; charset=utf-8");
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

    // ── GET /todos ── 一覧取得 ───────────────
    svr.Get("/todos", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        MyTodo db(DB_PATH);
        db.todos_list_handler(res);
    });

    // ── POST /todos ── 新規作成 ──────────────
    svr.Post("/todos", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        MyTodo db(DB_PATH); 
        db.todos_add_handler(req, res);
    });

    // ── PUT /todos/:id ── 更新（title / done） ─
    svr.Put(R"(/todos/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        MyTodo db(DB_PATH);
        db.todos_update_handler(req, res);
    });

    // ── DELETE /todos/:id ── 削除 ────────────
    svr.Delete(R"(/todos/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        MyTodo db(DB_PATH);
        db.todos_delete_handler(req, res);
    });

    // ── 起動 ────────────────────────────────
    int port_no = 8000;
    std::cout << "TODO Server running on http://localhost:" << port_no << "\n";
    std::cout << "Endpoints:\n"
              << "  GET    /todos\n"
              << "  POST   /todos        body: {\"title\":\"...\"}\n"
              << "  PUT    /todos/:id    body: {\"title\":\"...\",\"done\":true}\n"
              << "  DELETE /todos/:id\n";

    svr.listen("0.0.0.0", port_no);
    return 0;
}
