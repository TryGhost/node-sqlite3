/*
Copyright (c) 2010, Orlando Vazquez <ovazquez@gmail.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef DATABASE_H
#define DATABASE_H

#include <v8.h>
#include <node.h>
#include <node_events.h>

#include <sqlite3.h>

using namespace v8;
using namespace node;

class Database : public EventEmitter {
  public:
    static Persistent<FunctionTemplate> constructor_template;
    static void Init(v8::Handle<Object> target);

  protected:
    Database() : EventEmitter(), db_(NULL) { }

    ~Database() {
      assert(db_ == NULL);
      printf("Destroying database\n");
    }

    static Handle<Value> New(const Arguments& args);

    static int EIO_AfterOpen(eio_req *req);
    static int EIO_Open(eio_req *req);
    static Handle<Value> Open(const Arguments& args);

    static int EIO_AfterClose(eio_req *req);
    static int EIO_Close(eio_req *req);
    static Handle<Value> Close(const Arguments& args);

//     static Handle<Value> LastInsertRowid(const Arguments& args);
    static int EIO_AfterPrepareAndStep(eio_req *req);
    static int EIO_PrepareAndStep(eio_req *req);
    static Handle<Value> PrepareAndStep(const Arguments& args);

    static int EIO_AfterPrepare(eio_req *req);
    static int EIO_Prepare(eio_req *req);
    static Handle<Value> Prepare(const Arguments& args);

    // Return a pointer to the Sqlite handle pointer so that EIO_Open can
    // pass it to sqlite3_open which wants a pointer to an sqlite3 pointer. This
    // is because it wants to initialize our original (sqlite3*) pointer to
    // point to an valid object.
    sqlite3** GetDBPtr(void) { return &db_; }

  protected:

    sqlite3* db_;
};

enum ExecMode
{
    EXEC_EMPTY = 0,
    EXEC_LAST_INSERT_ID = 1,
    EXEC_AFFECTED_ROWS = 2
};

struct open_request {
  Persistent<Function> cb;
  Database *dbo;
  char filename[1];
};

struct close_request {
  Persistent<Function> cb;
  Database *dbo;
};

struct prepare_request {
  Persistent<Function> cb;
  Database *dbo;
  sqlite3_stmt* stmt;
  int mode;
  sqlite3_int64 lastInsertId;
  int affectedRows;
  const char* tail;
  char sql[1];
};

#endif
