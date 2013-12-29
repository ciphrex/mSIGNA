///////////////////////////////////////////////////////////////////////////////
//
// sqlite3cpp.h
//
// Copyright (c) 2011-2013 Eric Lombrozo
//
// C++ wrapper class for sqlite3
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef _SQLITE3CPP_QROISK_H
#define _SQLITE3CPP_QROISK_H

#include "sqlite3.h"

#include <string>
#include <fstream>
#include <stdexcept>

#ifdef SQLITE3CPP_LOG
#include <iostream>
#endif

typedef int (*fsqlite3_callback)(void*, int, char**, char**);


class SQLite3Exception : public std::runtime_error
{
private:
    int _code;

public:
    SQLite3Exception(const std::string& msg, int code = -1) : std::runtime_error(msg), _code(code) { }
    int code() const { return _code; } 
};

class SQLite3Stmt;

/////////////////////////////////////////////////
//
// class SQLite3DB
//
class SQLite3DB
{
private:
    sqlite3* db;

public:
    SQLite3DB() : db(NULL) { }
    SQLite3DB(const std::string& filename, bool bCreateIfNotExists = true) : db(NULL) { open(filename, bCreateIfNotExists); }
    ~SQLite3DB() { close(); }

    void open(const std::string& filename, bool bCreateIfNotExists = true);
    void close();
    bool isOpen() const { return (db != NULL); }

    void query(const std::string& sql, fsqlite3_callback callback = NULL, void* param = NULL) const;
    sqlite3_int64 lastInsertRowId() const;

    friend class SQLite3Stmt;
};

void SQLite3DB::open(const std::string& filename, bool bCreateIfNotExists)
{
    if (db) {
        throw SQLite3Exception("Database is already open.");
    }

    if (!bCreateIfNotExists) {
        std::ifstream ifile(filename);
        if (!ifile) {
            throw SQLite3Exception("File not found.");
        }
    }

    int res = sqlite3_open(filename.c_str(), &db);
    if (res != SQLITE_OK) {
        std::string err(sqlite3_errmsg(db));
        db = NULL;
        throw SQLite3Exception(err, res);
    }
}

void SQLite3DB::close()
{
    if (!db) return;

    int res = sqlite3_close(db);
    if (res != SQLITE_OK) {
        throw SQLite3Exception(sqlite3_errmsg(db), res); 
    }
    db = NULL;
}

void SQLite3DB::query(const std::string& sql, fsqlite3_callback callback, void* param) const
{
    if (!db) {
        throw SQLite3Exception("Database is not open.");
    }

#ifdef SQLITE3CPP_LOG
    std::cout << "SQLite3DB::query() - " << sql << std::endl;
#endif

    char* pErrMsg = NULL;
    int res = sqlite3_exec(db, sql.c_str(), callback, param, &pErrMsg);
    if (res != SQLITE_OK) {
        std::string err(pErrMsg);
        sqlite3_free(pErrMsg);
        throw SQLite3Exception(err, res);
    }
}

sqlite3_int64 SQLite3DB::lastInsertRowId() const
{
    if (!db) {
        throw SQLite3Exception("Database is not open.");
    }

    return sqlite3_last_insert_rowid(db);
}


/////////////////////////////////////////////////
//
// class SQLite3Stmt
//
class SQLite3Stmt
{
private:
    sqlite3_stmt* stmt;

public:
    SQLite3Stmt() : stmt(NULL) { }
    SQLite3Stmt(SQLite3DB& db, const std::string& sql) : stmt(NULL) { prepare(db, sql); }
    ~SQLite3Stmt() { finalize(); }

    void prepare(SQLite3DB& db, const std::string& sql);
    int step();
    void finalize();
    void reset();

    int getType(int iCol) { return sqlite3_column_type(stmt, iCol); }

    double getDouble(int iCol) { return sqlite3_column_double(stmt, iCol); }
    int getInt(int iCol) { return sqlite3_column_int(stmt, iCol); }
    sqlite3_int64 getInt64(int iCol) { return sqlite3_column_int64(stmt, iCol); }
    const unsigned char* getText(int iCol) { return sqlite3_column_text(stmt, iCol); }
};

void SQLite3Stmt::prepare(SQLite3DB& db, const std::string& sql)
{
    if (!db.db) {
        throw SQLite3Exception("Database is not open.");
    }

    if (stmt) {
        throw SQLite3Exception("Statement is already prepared.");
    }

#ifdef SQLITE3CPP_LOG
    std::cout << "SQLite3Stmt::prepare() - " << sql << std::endl;
#endif

    int res = sqlite3_prepare_v2(db.db, sql.c_str(), -1, &stmt, NULL);
    if (res != SQLITE_OK) {
        throw SQLite3Exception(sqlite3_errmsg(db.db), res);
    }
}

int SQLite3Stmt::step()
{
    if (!stmt) {
        throw SQLite3Exception("Statement is not prepared.");
    }

    int res = sqlite3_step(stmt);
    if (res == SQLITE_BUSY || res == SQLITE_DONE || res == SQLITE_ROW) {
        return res;
    }

    std::string err(sqlite3_errstr(res));
    throw SQLite3Exception(err, res); 
}

void SQLite3Stmt::reset()
{
    if (!stmt) {
        throw SQLite3Exception("Statement is not prepared.");
    }

    int res = sqlite3_reset(stmt);
    if (res != SQLITE_OK) {
        std::string err(sqlite3_errstr(res));
        throw SQLite3Exception(err);
    }
}

void SQLite3Stmt::finalize()
{
    if (!stmt) return;

    int res = sqlite3_finalize(stmt);
    if (res != SQLITE_OK) {
        std::string err(sqlite3_errstr(res));
        throw SQLite3Exception(err);
    }
    stmt = NULL;
}

/////////////////////////////////////////////////
//
// class SQLite3Tx
//
class SQLite3Tx
{
private:
    SQLite3DB& db;
    bool bCommitted;

public:
    enum Type { DEFERRED, IMMEDIATE, EXCLUSIVE };
    SQLite3Tx(SQLite3DB& _db, Type type = Type::DEFERRED);
    ~SQLite3Tx();

    void commit();
};

SQLite3Tx::SQLite3Tx(SQLite3DB& _db, Type type) : db(_db), bCommitted(false)
{
    std::string sql("BEGIN ");

    switch (type) {
    case (Type::DEFERRED):
        sql += "DEFERRED";
        break;
    case (Type::IMMEDIATE):
        sql += "IMMEDIATE";
        break;
    case (Type::EXCLUSIVE):
        sql += "EXCLUSIVE";
        break;
    default:
        throw SQLite3Exception("Invalid Sqlite3 transaction type.");
    }

    db.query(sql);
}

SQLite3Tx::~SQLite3Tx()
{
    if (!bCommitted) {
        db.query("ROLLBACK");
    }    
}

void SQLite3Tx::commit()
{
    db.query("COMMIT");
    bCommitted = true;
}

#endif // _SQLITE3CPP_QROISK_H
