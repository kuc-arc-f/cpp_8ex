#include "httplib.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp> // JSONライブラリ
#include <vector>
#include <string>
#include <sqlite3.h>
#include <sstream>
#include <dotenv.h>

#include "my_todo.hpp"
#include "my_page.hpp"

using json = nlohmann::json;

std::string DB_PATH = "./todo.db";

// インメモリストレージ
static std::vector<Todo> g_todos;
static std::mutex        g_mutex;


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
const std::string COOKIE_KEY_USER = "http_8user";

// ─────────────────────────────────────────
// main
// ─────────────────────────────────────────
int main() {
    httplib::Server svr;
    dotenv::init();

    std::cout << "db_path=" << DB_PATH << " \n";

    // ── OPTIONS（プリフライト） ──────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        set_cors(res);
        res.status = 204;
    });
    /* API */   
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

    // ─── SSR HTMLページ (CSSを<link>タグで参照) ───
    svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        const char* valid_login = std::getenv("VALID_LOGIN");
        if (valid_login && valid_login == std::string("true")) {
            std::cout << "valid_login=" << valid_login << std::endl;
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
        //MyPage app("");
        //app.root_page_handler(req, res);
        res.set_file_content("./html/root.html");
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
