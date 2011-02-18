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

#include <string>
#include <queue>

#include <sqlite3.h>

using namespace v8;
using namespace node;

class Statement : public EventEmitter {
public:
    static Persistent<FunctionTemplate> constructor_template;

    static void Init(Handle<Object> target);
    static Handle<Value> New(const Arguments& args);

    static struct Baton {
        Statement* stmt;
        Persistent<Function> callback;
        int status;
        std::string message;

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



    void Schedule(EIO_Callback callback, Baton* baton);
    void Process();
    void CleanQueue();

    static Handle<Value> Finalize(const Arguments& args);
    static void Finalize(Baton* baton);
    void Finalize();
protected:
    Database* db;

    sqlite3_stmt* handle;

    bool prepared;
    bool locked;
    bool finalized;
    std::queue<Call*> queue;
};

#endif
