#pragma once
#include <cstdlib>
#include <iostream>
#include <libpq-fe.h>

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
