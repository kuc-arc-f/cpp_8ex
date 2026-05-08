#pragma once
#include <iostream>

class MyPage {
private:
    std::string m_name = "";

public:
    explicit MyPage(std::string str){ m_name = str; }

    ~MyPage() {}


    /**
    *
    * @param
    *
    * @return
    */
    void root_page_handler(const httplib::Request& req, httplib::Response& res) {
        std::string ret = "";
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
    }

};

