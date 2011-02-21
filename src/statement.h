// Copyright (c) 2010, Orlando Vazquez <ovazquez@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef NODE_SQLITE3_SRC_STATEMENT_H
#define NODE_SQLITE3_SRC_STATEMENT_H

#include <v8.h>
#include <node.h>
#include <node_events.h>

#include "database.h"

#include <typeinfo>
#include <string>
#include <queue>
#include <vector>

#include <sqlite3.h>

using namespace v8;
using namespace node;

namespace node_sqlite3 {

namespace Data {
    struct Field {
        Field(unsigned short _type = SQLITE_NULL) : type(_type) {}
        unsigned short type;
    };

    struct Integer : Field {
        Integer(int val) : Field(SQLITE_INTEGER), value(val) {}
        int value;
    };

    struct Float : Field {
        Float(double val) : Field(SQLITE_FLOAT), value(val) {}
        double value;
    };

    struct Text : Field {
        Text(size_t len, const char* val) : Field(SQLITE_TEXT), value(val, len) {}
        std::string value;
    };

    struct Blob : Field {
        Blob(size_t len, const void* val) : Field(SQLITE_BLOB), length(len) {
            value = malloc(len);
            memcpy(value, val, len);
        }
        ~Blob() {
            free(value);
        }
        int length;
        void* value;
    };

    typedef Field Null;
    typedef std::vector<Field*> Row;
    typedef Row Parameters;
    typedef std::vector<Row*> Rows;
}




class Statement : public EventEmitter {
public:
    static Persistent<FunctionTemplate> constructor_template;

    static void Init(Handle<Object> target);
    static Handle<Value> New(const Arguments& args);

    static struct Baton {
        Statement* stmt;
        Persistent<Function> callback;
        Data::Parameters parameters;

        Baton(Statement* stmt_, Handle<Function> cb_) : stmt(stmt_) {
            stmt->Ref();
            ev_ref(EV_DEFAULT_UC);
            callback = Persistent<Function>::New(cb_);
        }
        ~Baton() {
            stmt->Unref();
            ev_unref(EV_DEFAULT_UC);
            callback.Dispose();
        }
    };

    static struct RowBaton : Baton {
        RowBaton(Statement* stmt_, Handle<Function> cb_) :
            Baton(stmt_, cb_) {}
        Data::Row row;
    };

    static struct PrepareBaton : Database::Baton {
        Statement* stmt;
        std::string sql;
        PrepareBaton(Database* db_, Handle<Function> cb_, Statement* stmt_) :
            Baton(db_, cb_), stmt(stmt_) {
            stmt->Ref();
        }
        ~PrepareBaton() {
            stmt->Unref();
        }
    };

    typedef void (*EIO_Callback)(Baton* baton);

    struct Call {
        Call(EIO_Callback cb_, Baton* baton_) : callback(cb_), baton(baton_) {};
        EIO_Callback callback;
        Baton* baton;
    };

    Statement(Database* db_) : EventEmitter(),
        db(db_),
        handle(NULL),
        status(SQLITE_OK),
        prepared(false),
        locked(false),
        finalized(false) {
        db->pending++;
        db->Ref();
    }

    ~Statement() {
        if (!finalized) Finalize();
    }

protected:
    static void EIO_BeginPrepare(Database::Baton* baton);
    static int EIO_Prepare(eio_req *req);
    static int EIO_AfterPrepare(eio_req *req);

    EIO_DEFINITION(Bind);
    EIO_DEFINITION(Get);
    EIO_DEFINITION(Run);
    EIO_DEFINITION(All);
    EIO_DEFINITION(Reset);

    static Handle<Value> Finalize(const Arguments& args);
    static void Finalize(Baton* baton);
    void Finalize();

    template <class T> T* Bind(const Arguments& args, int start = 0);
    bool Bind(const Data::Parameters parameters);

    static void GetRow(Data::Row* row, sqlite3_stmt* stmt);
    static Local<Array> RowToJS(Data::Row* row);
    void Schedule(EIO_Callback callback, Baton* baton);
    void Process();
    void CleanQueue();
    template <class T> static void Error(T* baton);

protected:
    Database* db;

    sqlite3_stmt* handle;
    int status;
    std::string message;

    bool prepared;
    bool locked;
    bool finalized;
    std::queue<Call*> queue;
};

}

#endif
