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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <v8.h>
#include <node.h>
#include <node_events.h>

#include <sqlite3.h>

#include "sqlite3_bindings.h"
#include "statement.h"

using namespace v8;
using namespace node;

// static Persistent<String> callback_sym;

class Sqlite3Db : public EventEmitter {
public:
  static Persistent<FunctionTemplate> constructor_template;
  static void Init(v8::Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(EventEmitter::constructor_template);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("Database"));

    NODE_SET_PROTOTYPE_METHOD(constructor_template, "open", Open);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "close", Close);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "prepare", Prepare);
//     NODE_SET_PROTOTYPE_METHOD(constructor_template, "changes", Changes);
//     NODE_SET_PROTOTYPE_METHOD(constructor_template, "lastInsertRowid", LastInsertRowid);

    target->Set(v8::String::NewSymbol("Database"),
            constructor_template->GetFunction());

    // insert/update execution result mask
    NODE_DEFINE_CONSTANT (target, EXEC_EMPTY);
    NODE_DEFINE_CONSTANT (target, EXEC_LAST_INSERT_ID);
    NODE_DEFINE_CONSTANT (target, EXEC_AFFECTED_ROWS);
    
    Statement::Init(target);
  }

protected:
  Sqlite3Db() : EventEmitter(), db_(NULL) { }

  ~Sqlite3Db() {
    assert(db_ == NULL);
  }

  sqlite3* db_;

  // Return a pointer to the Sqlite handle pointer so that EIO_Open can
  // pass it to sqlite3_open which wants a pointer to an sqlite3 pointer. This
  // is because it wants to initialize our original (sqlite3*) pointer to
  // point to an valid object.
  sqlite3** GetDBPtr(void) { return &db_; }

protected:

  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* dbo = new Sqlite3Db();
    dbo->Wrap(args.This());
    return args.This();
  }

  // To pass arguments to the functions that will run in the thread-pool we
  // have to pack them into a struct and pass eio_custom a pointer to it.

  struct open_request {
    Persistent<Function> cb;
    Sqlite3Db *dbo;
    char filename[1];
  };

  static int EIO_AfterOpen(eio_req *req) {
    ev_unref(EV_DEFAULT_UC);
    HandleScope scope;
    struct open_request *open_req = (struct open_request *)(req->data);

    Local<Value> argv[1];
    bool err = false;
    if (req->result) {
      err = true;
      argv[0] = Exception::Error(String::New("Error opening database"));
    }

    TryCatch try_catch;

    open_req->dbo->Unref();
    open_req->cb->Call(Context::GetCurrent()->Global(), err ? 1 : 0, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    open_req->dbo->Emit(String::New("ready"), 0, NULL);
    open_req->cb.Dispose();

    free(open_req);

    return 0;
  }

  static int EIO_Open(eio_req *req) {
    struct open_request *open_req = (struct open_request *)(req->data);

    sqlite3 **dbptr = open_req->dbo->GetDBPtr();
    int rc = sqlite3_open_v2( open_req->filename
                            , dbptr
                            , SQLITE_OPEN_READWRITE
                              | SQLITE_OPEN_CREATE
                              | SQLITE_OPEN_FULLMUTEX
                            , NULL);

    req->result = rc;

//     sqlite3 *db = *dbptr;
//     sqlite3_commit_hook(db, CommitHook, open_req->dbo);
//     sqlite3_rollback_hook(db, RollbackHook, open_req->dbo);
//     sqlite3_update_hook(db, UpdateHook, open_req->dbo);

    return 0;
  }

  static Handle<Value> Open(const Arguments& args) {
    HandleScope scope;

    REQ_STR_ARG(0, filename);
    REQ_FUN_ARG(1, cb);

    Sqlite3Db* dbo = ObjectWrap::Unwrap<Sqlite3Db>(args.This());

    struct open_request *open_req = (struct open_request *)
        calloc(1, sizeof(struct open_request) + filename.length());

    if (!open_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
        String::New("Could not allocate enough memory")));
    }

    strcpy(open_req->filename, *filename);
    open_req->cb = Persistent<Function>::New(cb);
    open_req->dbo = dbo;

    eio_custom(EIO_Open, EIO_PRI_DEFAULT, EIO_AfterOpen, open_req);

    ev_ref(EV_DEFAULT_UC);
    dbo->Ref();

    return Undefined();
  }

  //
  // JS DatabaseSync bindings
  //

  // TODO: libeio'fy
  static Handle<Value> Changes(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* dbo = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    Local<Number> result = Integer::New(sqlite3_changes(dbo->db_));
    return scope.Close(result);
  }

  struct close_request {
    Persistent<Function> cb;
    Sqlite3Db *dbo;
  };

  static int EIO_AfterClose(eio_req *req) {
    ev_unref(EV_DEFAULT_UC);

    HandleScope scope;

    struct close_request *close_req = (struct close_request *)(req->data);

    Local<Value> argv[1];
    bool err = false;
    if (req->result) {
      err = true;
      argv[0] = Exception::Error(String::New("Error closing database"));
    }

    TryCatch try_catch;

    close_req->dbo->Unref();
    close_req->cb->Call(Context::GetCurrent()->Global(), err ? 1 : 0, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    close_req->cb.Dispose();

    free(close_req);

    return 0;
  }

  static int EIO_Close(eio_req *req) {
    struct close_request *close_req = (struct close_request *)(req->data);
    Sqlite3Db* dbo = close_req->dbo;
    req->result = sqlite3_close(dbo->db_);
    dbo->db_ = NULL;
    return 0;
  }

  static Handle<Value> Close(const Arguments& args) {
    HandleScope scope;

    REQ_FUN_ARG(0, cb);

    Sqlite3Db* dbo = ObjectWrap::Unwrap<Sqlite3Db>(args.This());

    struct close_request *close_req = (struct close_request *)
        calloc(1, sizeof(struct close_request));

    if (!close_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
        String::New("Could not allocate enough memory")));
    }

    close_req->cb = Persistent<Function>::New(cb);
    close_req->dbo = dbo;

    eio_custom(EIO_Close, EIO_PRI_DEFAULT, EIO_AfterClose, close_req);

    ev_ref(EV_DEFAULT_UC);
    dbo->Ref();

    return Undefined();
  }

  // TODO: libeio'fy
  static Handle<Value> LastInsertRowid(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* dbo = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    Local<Number> result = Integer::New(sqlite3_last_insert_rowid(dbo->db_));
    return scope.Close(result);
  };

  // Hooks

//   static int CommitHook(void* v_this) {
//     HandleScope scope;
//     Sqlite3Db* db = static_cast<Sqlite3Db*>(v_this);
//     db->Emit(String::New("commit"), 0, NULL);
//     // TODO: allow change in return value to convert to rollback...somehow
//     return 0;
//   }
//
//   static void RollbackHook(void* v_this) {
//     HandleScope scope;
//     Sqlite3Db* db = static_cast<Sqlite3Db*>(v_this);
//     db->Emit(String::New("rollback"), 0, NULL);
//   }
//
//   static void UpdateHook(void* v_this, int operation, const char* database,
//                          const char* table, sqlite_int64 rowid) {
//     HandleScope scope;
//     Sqlite3Db* db = static_cast<Sqlite3Db*>(v_this);
//     Local<Value> args[] = { Int32::New(operation), String::New(database),
//                             String::New(table), Number::New(rowid) };
//     db->Emit(String::New("update"), 4, args);
//   }

  struct prepare_request {
    Persistent<Function> cb;
    Sqlite3Db *dbo;
    sqlite3_stmt* stmt;
    int mode;
    sqlite3_int64 lastInsertId;
    int affectedRows;
    const char* tail;
    char sql[1];
  };

  static int EIO_AfterPrepare(eio_req *req) {
    ev_unref(EV_DEFAULT_UC);
    struct prepare_request *prep_req = (struct prepare_request *)(req->data);
    HandleScope scope;

    Local<Value> argv[2];
    int argc = 0;

    // if the prepare failed
    if (req->result != SQLITE_OK) {
      argv[0] = Exception::Error(
                  String::New(sqlite3_errmsg(prep_req->dbo->db_)));
      argc = 1;

    } else {
      if (req->int1 == SQLITE_DONE) {

          if (prep_req->mode != EXEC_EMPTY) {
            argv[0] = Local<Value>::New(Undefined());   // no error

            Local<Object> info = Object::New();         

            if (prep_req->mode & EXEC_LAST_INSERT_ID) {
                info->Set(String::NewSymbol("last_inserted_id"),
                    Integer::NewFromUnsigned (prep_req->lastInsertId));
            }
            if (prep_req->mode & EXEC_AFFECTED_ROWS) {
                info->Set(String::NewSymbol("affected_rows"),
                          Integer::New (prep_req->affectedRows));
            }
            argv[1] = info;
            argc = 2;
            
        } else {
            argc = 0;
        }
        
      } else {
        argv[0] = External::New(prep_req->stmt);
        argv[1] = Integer::New(req->int1);
        Persistent<Object> statement(
          Statement::constructor_template->GetFunction()->NewInstance(2, argv));

        if (prep_req->tail) {
          statement->Set(String::New("tail"), String::New(prep_req->tail));
        }

        argv[0] = Local<Value>::New(Undefined());
        argv[1] = Local<Value>::New(statement);
        argc = 2;
      }
    }

    TryCatch try_catch;

    prep_req->dbo->Unref();
    prep_req->cb->Call(Context::GetCurrent()->Global(), argc, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    prep_req->cb.Dispose();
    free(prep_req);

    return 0;
  }

  static int EIO_Prepare(eio_req *req) {
    struct prepare_request *prep_req = (struct prepare_request *)(req->data);

    prep_req->stmt = NULL;
    prep_req->tail = NULL;
    sqlite3* db = prep_req->dbo->db_;

    int rc = sqlite3_prepare_v2(db, prep_req->sql, -1,
                &(prep_req->stmt), &(prep_req->tail));

    req->result = rc;
    req->int1 = -1;

    // This might be a INSERT statement. Let's try to get the first row.
    // This is to optimize out further calls to the thread pool. This is only
    // possible in the case where there are no variable placeholders/bindings
    // in the SQL.
    if (rc == SQLITE_OK && !sqlite3_bind_parameter_count(prep_req->stmt)) {
      rc = sqlite3_step(prep_req->stmt);
      req->int1 = rc;

      // no more rows to return, clean up statement
      if (rc == SQLITE_DONE) {
        rc = sqlite3_finalize(prep_req->stmt);
        prep_req->stmt = NULL;
        assert(rc == SQLITE_OK);
      }
    }

    prep_req->lastInsertId = 0;
    prep_req->affectedRows = 0;
        
    // load custom properties
    if (prep_req->mode & EXEC_LAST_INSERT_ID)
        prep_req->lastInsertId = sqlite3_last_insert_rowid(db);
    if (prep_req->mode & EXEC_AFFECTED_ROWS)
        prep_req->affectedRows = sqlite3_changes(db);

    return 0;
  }

  static Handle<Value> Prepare(const Arguments& args) {
    HandleScope scope;
    REQ_STR_ARG(0, sql);
    REQ_FUN_ARG(1, cb);
    OPT_INT_ARG(2, mode, EXEC_EMPTY);

    Sqlite3Db* dbo = ObjectWrap::Unwrap<Sqlite3Db>(args.This());

    struct prepare_request *prep_req = (struct prepare_request *)
        calloc(1, sizeof(struct prepare_request) + sql.length());

    if (!prep_req) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
        String::New("Could not allocate enough memory")));
    }

    strcpy(prep_req->sql, *sql);
    prep_req->cb = Persistent<Function>::New(cb);
    prep_req->dbo = dbo;
    prep_req->mode = mode;

    eio_custom(EIO_Prepare, EIO_PRI_DEFAULT, EIO_AfterPrepare, prep_req);

    ev_ref(EV_DEFAULT_UC);
    dbo->Ref();

    return Undefined();
  }
};

Persistent<FunctionTemplate> Sqlite3Db::constructor_template;

extern "C" void init (v8::Handle<Object> target) {
  Sqlite3Db::Init(target);
}

