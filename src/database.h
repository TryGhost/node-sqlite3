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

#ifndef NODE_SQLITE3_SRC_DATABASE_H
#define NODE_SQLITE3_SRC_DATABASE_H

#include <v8.h>
#include <node.h>
#include <node_events.h>

#include "deferred_call.h"

#include <string>
#include <queue>

#include <sqlite3.h>

using namespace v8;
using namespace node;

class Database : public EventEmitter {
  public:
    static Persistent<FunctionTemplate> constructor_template;
    static void Init(v8::Handle<Object> target);

    static enum Status {
        IsClosed     = 1 << 0,
        IsOpening    = 1 << 1,
        IsOpen       = 1 << 2,
        IsClosing    = 1 << 3,

        DoesntMatter = IsClosed | IsOpening | IsOpen | IsClosing
    };

    typedef Deferred::Call<Status> Call;

    struct Baton {
        Baton(Database* db_, Persistent<Function> callback_) :
            db(db_), callback(callback_) {};
        Database* db;
        Persistent<Function> callback;
    };

  protected:
    Database() : EventEmitter(),
        handle(NULL),
        pending(0),
        status(IsClosed) {

    }

    ~Database() {
        fprintf(stderr, "Calling destructor\n");
    }

    static Handle<Value> New(const Arguments& args);

    static void ProcessQueue(Database* db);

    static Handle<Value> OpenSync(const Arguments& args);
    static Handle<Value> Open(const Arguments& args);
    static bool Open(Database* db);
    static int EIO_Open(eio_req *req);
    static int EIO_AfterOpen(eio_req *req);

    static Handle<Value> CloseSync(const Arguments& args);
    static Handle<Value> Close(const Arguments& args);
    static bool Close(Database* db);
    static int EIO_Close(eio_req *req);
    static int EIO_AfterClose(eio_req *req);

    static int EIO_AfterPrepareAndStep(eio_req *req);
    static int EIO_PrepareAndStep(eio_req *req);
    static Handle<Value> PrepareAndStep(const Arguments& args);

    static int EIO_AfterPrepare(eio_req *req);
    static int EIO_Prepare(eio_req *req);
    static Handle<Value> Prepare(const Arguments& args);

    void Wrap (Handle<Object> handle);
    static void Destruct (Persistent<Value> value, void *data);
    static int EIO_Destruct(eio_req *req);
    static int EIO_AfterDestruct(eio_req *req);

  protected:
    sqlite3* handle;
    std::string filename;
    int open_mode;

    std::string error_message;
    int error_status;

    int pending;
    Status status;
    std::queue<Call*> queue;
  private:
};


enum ExecMode {
    EXEC_EMPTY = 0,
    EXEC_LAST_INSERT_ID = 1,
    EXEC_AFFECTED_ROWS = 2
};

struct prepare_request {
  Persistent<Function> cb;
  Database *db;
  sqlite3_stmt* stmt;
  int mode;
  sqlite3_int64 lastInsertId;
  int affectedRows;
  const char* tail;
  char sql[1];
};

#endif
