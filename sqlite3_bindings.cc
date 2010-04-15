/*
Copyright (c) 2009, Eric Fredricksen <e@fredricksen.net>
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

using namespace v8;
using namespace node;

static Persistent<String> callback_sym;

#define CHECK(rc) { if ((rc) != SQLITE_OK)                              \
      return ThrowException(Exception::Error(String::New(               \
                                             sqlite3_errmsg(*db)))); }

#define SCHECK(rc) { if ((rc) != SQLITE_OK)                             \
      return ThrowException(Exception::Error(String::New(               \
                        sqlite3_errmsg(sqlite3_db_handle(sto->stmt_))))); }

#define REQ_ARGS(N)                                                     \
  if (args.Length() < (N))                                              \
    return ThrowException(Exception::TypeError(                         \
                             String::New("Expected " #N "arguments")));

#define REQ_STR_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsString())                     \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a string")));    \
  String::Utf8Value VAR(args[I]->ToString());

#define REQ_FUN_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsFunction())                   \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a function")));  \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

#define REQ_EXT_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsExternal())                   \
    return ThrowException(Exception::TypeError(                         \
                              String::New("Argument " #I " invalid"))); \
  Local<External> VAR = Local<External>::Cast(args[I]);

#define OPT_INT_ARG(I, VAR, DEFAULT)                                    \
  int VAR;                                                              \
  if (args.Length() <= (I)) {                                           \
    VAR = (DEFAULT);                                                    \
  } else if (args[I]->IsInt32()) {                                      \
    VAR = args[I]->Int32Value();                                        \
  } else {                                                              \
    return ThrowException(Exception::TypeError(                         \
              String::New("Argument " #I " must be an integer")));      \
  }

enum ExecMode
{
    EXEC_EMPTY = 0,
    EXEC_LAST_INSERT_ID = 1,
    EXEC_AFFECTED_ROWS = 2
};

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

  class Statement : public EventEmitter {

  public:
    static Persistent<FunctionTemplate> constructor_template;

    static void Init(v8::Handle<Object> target) {
      HandleScope scope;

      Local<FunctionTemplate> t = FunctionTemplate::New(New);

      constructor_template = Persistent<FunctionTemplate>::New(t);
      constructor_template->Inherit(EventEmitter::constructor_template);
      constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
      constructor_template->SetClassName(String::NewSymbol("Statement"));

      NODE_SET_PROTOTYPE_METHOD(t, "bind", Bind);
//       NODE_SET_PROTOTYPE_METHOD(t, "clearBindings", ClearBindings);
      NODE_SET_PROTOTYPE_METHOD(t, "finalize", Finalize);
//       NODE_SET_PROTOTYPE_METHOD(t, "bindParameterCount", BindParameterCount);
//       NODE_SET_PROTOTYPE_METHOD(t, "reset", Reset);
      NODE_SET_PROTOTYPE_METHOD(t, "step", Step);

      callback_sym = Persistent<String>::New(String::New("callback"));

      //target->Set(v8::String::NewSymbol("SQLStatement"), t->GetFunction());
    }

    static Handle<Value> New(const Arguments& args) {
      HandleScope scope;
      REQ_EXT_ARG(0, stmt);
      int first_rc = args[1]->IntegerValue();

      Statement *sto = new Statement((sqlite3_stmt*)stmt->Value(), first_rc);
      sto->Wrap(args.This());
      sto->Ref();

      return args.This();
    }

  protected:
    Statement(sqlite3_stmt* stmt, int first_rc = -1)
    : EventEmitter(), first_rc_(first_rc), stmt_(stmt) {
      column_count_ = -1;
      column_types_ = NULL;
      column_names_ = NULL;
      column_data_ = NULL;
    }

    ~Statement() {
      if (stmt_) sqlite3_finalize(stmt_);
      if (column_types_) free(column_types_);
      if (column_names_) free(column_names_);
      if (column_data_) FreeColumnData();
    }

    //
    // JS prepared statement bindings
    //

    // indicate the key type (integer index or name string)
    enum BindKeyType {
      KEY_INT,
      KEY_STRING
    };

    // indicate the parameter type
    enum BindValueType {
      VALUE_INT,
      VALUE_DOUBLE,
      VALUE_STRING,
      VALUE_NULL
    };

    struct bind_request {
      Persistent<Function> cb;
      Statement *sto;

      enum BindKeyType   key_type;
      enum BindValueType value_type;

      void *key;   // char * | int *
      void *value; // char * | int * | double * | 0
      size_t value_size;
    };

    static int EIO_AfterBind(eio_req *req) {
      ev_unref(EV_DEFAULT_UC);

      HandleScope scope;
      struct bind_request *bind_req = (struct bind_request *)(req->data);

      Local<Value> argv[1];
      bool err = false;
      if (req->result) {
        err = true;
        argv[0] = Exception::Error(String::New("Error binding parameter"));
      }

      TryCatch try_catch;

      bind_req->cb->Call(Context::GetCurrent()->Global(), err ? 1 : 0, argv);

      if (try_catch.HasCaught()) {
        FatalException(try_catch);
      }

      bind_req->cb.Dispose();

      free(bind_req->key);
      free(bind_req->value);
      free(bind_req);

      return 0;
    }

    static int EIO_Bind(eio_req *req) {
      struct bind_request *bind_req = (struct bind_request *)(req->data);
      Statement *sto = bind_req->sto;

      int index(0);
      switch(bind_req->key_type) {
        case KEY_INT:
          index = *(int*)(bind_req->key);
          break;

        case KEY_STRING:
          index = sqlite3_bind_parameter_index(sto->stmt_,
                                               (char*)(bind_req->key));
          break;

         default: {
           // this SHOULD be unreachable
         }
      }

      if (!index) {
        req->result = SQLITE_MISMATCH;
        return 0;
      }

      int rc = 0;
      switch(bind_req->value_type) {
        case VALUE_INT:
          rc = sqlite3_bind_int(sto->stmt_, index, *(int*)(bind_req->value));
          break;
        case VALUE_DOUBLE:
          rc = sqlite3_bind_double(sto->stmt_, index, *(double*)(bind_req->value));
          break;
        case VALUE_STRING:
          rc = sqlite3_bind_text(sto->stmt_, index, (char*)(bind_req->value),
                            bind_req->value_size, SQLITE_TRANSIENT);
          break;
        case VALUE_NULL:
          rc = sqlite3_bind_null(sto->stmt_, index);
          break;

        default: {
          // should be unreachable
        }
      }

      if (rc) {
        req->result = rc;
        return 0;
      }

      return 0;
    }

    static Handle<Value> Bind(const Arguments& args) {
      HandleScope scope;
      Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

      REQ_ARGS(2);
      REQ_FUN_ARG(2, cb);

      if (!args[0]->IsString() && !args[0]->IsInt32())
        return ThrowException(Exception::TypeError(
               String::New("First argument must be a string or integer")));

      struct bind_request *bind_req = (struct bind_request *)
          calloc(1, sizeof(struct bind_request));

      // setup key
      if (args[0]->IsString()) {
         String::Utf8Value keyValue(args[0]);
         bind_req->key_type = KEY_STRING;

         char *key = (char *) calloc(1, keyValue.length() + 1);
         bind_req->value_size = keyValue.length() + 1;
         strcpy(key, *keyValue);

         bind_req->key = key;
      }
      else if (args[0]->IsInt32()) {
        bind_req->key_type = KEY_INT;
        int *index = (int *) malloc(sizeof(int));
        *index = args[0]->Int32Value();
        bind_req->value_size = sizeof(int);

        // don't forget to `free` this
        bind_req->key = index;
      }

      // setup value
      if (args[1]->IsInt32()) {
        bind_req->value_type = VALUE_INT;
        int *value = (int *) malloc(sizeof(int));
        *value = args[1]->Int32Value();
        bind_req->value = value;
      }
      else if (args[1]->IsNumber()) {
        bind_req->value_type = VALUE_DOUBLE;
        double *value = (double *) malloc(sizeof(double));
        *value = args[1]->NumberValue();
        bind_req->value = value;
      }
      else if (args[1]->IsString()) {
        bind_req->value_type = VALUE_STRING;
        String::Utf8Value text(args[1]);
        char *value = (char *) calloc(text.length()+1, sizeof(char*));
        strcpy(value, *text);
        bind_req->value = value;
        bind_req->value_size = text.length()+1;
      }
      else if (args[1]->IsNull() || args[1]->IsUndefined()) {
        bind_req->value_type = VALUE_NULL;
        bind_req->value = NULL;
      }
      else {
        free(bind_req->key);
        return ThrowException(Exception::TypeError(
               String::New("Unable to bind value of this type")));
      }

      bind_req->cb = Persistent<Function>::New(cb);
      bind_req->sto = sto;

      eio_custom(EIO_Bind, EIO_PRI_DEFAULT, EIO_AfterBind, bind_req);

      ev_ref(EV_DEFAULT_UC);

      return Undefined();
    }

    // XXX TODO
    static Handle<Value> BindParameterCount(const Arguments& args) {
      HandleScope scope;
      Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());
      Local<Number> result = Integer::New(sqlite3_bind_parameter_count(sto->stmt_));
      return scope.Close(result);
    }

    static Handle<Value> ClearBindings(const Arguments& args) {
      HandleScope scope;
      Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());
      SCHECK(sqlite3_clear_bindings(sto->stmt_));
      return Undefined();
    }

    static int EIO_AfterFinalize(eio_req *req) {
      ev_unref(EV_DEFAULT_UC);
      Statement *sto = (class Statement *)(req->data);

      HandleScope scope;

      Local<Function> cb = sto->GetCallback();

      TryCatch try_catch;

      cb->Call(sto->handle_, 0, NULL);

      if (try_catch.HasCaught()) {
        FatalException(try_catch);
      }

      sto->Unref();

      return 0;
    }

    static int EIO_Finalize(eio_req *req) {
      Statement *sto = (class Statement *)(req->data);

      assert(sto->stmt_);
      req->result = sqlite3_finalize(sto->stmt_);
      sto->stmt_ = NULL;

      return 0;
    }

    static Handle<Value> Finalize(const Arguments& args) {
      HandleScope scope;

      Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

      if (sto->HasCallback()) {
        return ThrowException(Exception::Error(String::New("Already stepping")));
      }

      REQ_FUN_ARG(0, cb);

      sto->SetCallback(cb);

      eio_custom(EIO_Finalize, EIO_PRI_DEFAULT, EIO_AfterFinalize, sto);

      ev_ref(EV_DEFAULT_UC);

      return Undefined();
    }

//     static Handle<Value> Reset(const Arguments& args) {
//       HandleScope scope;
//       Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());
//       SCHECK(sqlite3_reset(sto->stmt_));
//       return Undefined();
//     }

    static int EIO_AfterStep(eio_req *req) {
      ev_unref(EV_DEFAULT_UC);

      HandleScope scope;

      Statement *sto = (class Statement *)(req->data);

      Local<Value> argv[2];

      if (sto->error_) {
        argv[0] = Exception::Error(
            String::New(sqlite3_errmsg(sqlite3_db_handle(sto->stmt_))));
      }
      else {
        argv[0] = Local<Value>::New(Undefined());
      }

      if (req->result == SQLITE_DONE) {
        argv[1] = Local<Value>::New(Null());
      }
      else {
        Local<Object> row = Object::New();

        for (int i = 0; i < sto->column_count_; i++) {
          assert(sto->column_data_);
          assert(((void**)sto->column_data_)[i]);
          assert(sto->column_names_[i]);
          assert(sto->column_types_[i]);

          switch (sto->column_types_[i]) {
            // XXX why does using String::New make v8 croak here?
            case SQLITE_INTEGER:
              row->Set(String::NewSymbol((char*) sto->column_names_[i]),
                       Int32::New(*(int*) (sto->column_data_[i])));
              break;

            case SQLITE_FLOAT:
              row->Set(String::New(sto->column_names_[i]),
                       Number::New(*(double*) (sto->column_data_[i])));
              break;

            case SQLITE_TEXT:
              assert(strlen((char*)sto->column_data_[i]));
              row->Set(String::New(sto->column_names_[i]),
                       String::New((char *) (sto->column_data_[i])));
              // don't free this pointer, it's owned by sqlite3
              break;

            // no default
          }
        }

        argv[1] = row;
      }

      TryCatch try_catch;

      Local<Function> cb = sto->GetCallback();

      cb->Call(sto->handle_, 2, argv);

      if (try_catch.HasCaught()) {
        FatalException(try_catch);
      }

      if (req->result == SQLITE_DONE && sto->column_count_) {
        sto->FreeColumnData();
      }

      return 0;
    }

    void FreeColumnData(void) {
      if (!column_count_) return;
      for (int i = 0; i < column_count_; i++) {
        switch (column_types_[i]) {
          case SQLITE_INTEGER:
            free(column_data_[i]);
            break;
          case SQLITE_FLOAT:
            free(column_data_[i]);
            break;
        }
        column_data_[i] = NULL;
      }

      free(column_data_);
      column_data_ = NULL;
    }

    static int EIO_Step(eio_req *req) {
      Statement *sto = (class Statement *)(req->data);
      sqlite3_stmt *stmt = sto->stmt_;
      assert(stmt);
      int rc;

      // check if we have already taken a step immediately after prepare
      if (sto->first_rc_ != -1) {
        // This is the first one! Let's just use the rc from when we called
        // it in EIO_Prepare
        rc = req->result = sto->first_rc_;
        // Now we set first_rc_ to -1 so that on the next step, it won't
        // think this is the first.
        sto->first_rc_ = -1;
      }
      else {
        rc = req->result = sqlite3_step(stmt);
      }

      sto->error_ = false;

      if (rc == SQLITE_ROW) {
        // If these pointers are NULL, look up and store the number of columns
        // their names and types.
        // Otherwise that means we have already looked up the column types and
        // names so we can simply re-use that info.
        if (!sto->column_types_ && !sto->column_names_) {
          sto->column_count_ = sqlite3_column_count(stmt);
          assert(sto->column_count_);
          sto->column_types_ = (int *) calloc(sto->column_count_, sizeof(int));
          sto->column_names_ = (char **) calloc(sto->column_count_,
                                                sizeof(char *));

          if (sto->column_count_) {
            sto->column_data_ = (void **) calloc(sto->column_count_,
                                                 sizeof(void *));
          }

          for (int i = 0; i < sto->column_count_; i++) {
            sto->column_types_[i] = sqlite3_column_type(stmt, i);
            sto->column_names_[i] = (char *) sqlite3_column_name(stmt, i);

            switch(sto->column_types_[i]) {
              case SQLITE_INTEGER:
                sto->column_data_[i] = (int *) malloc(sizeof(int));
                break;

              case SQLITE_FLOAT:
                sto->column_data_[i] = (double *) malloc(sizeof(double));
                break;

              // no need to allocate memory for strings

              default: {
                // unsupported type
              }
            }
          }
        }

        assert(sto->column_types_ && sto->column_data_ && sto->column_names_);

        for (int i = 0; i < sto->column_count_; i++) {
          int type = sto->column_types_[i];

          switch(type) {
            case SQLITE_INTEGER: 
              *(int*)(sto->column_data_[i]) = sqlite3_column_int(stmt, i);
              assert(sto->column_data_[i]);
              break;

            case SQLITE_FLOAT:
              *(double*)(sto->column_data_[i]) = sqlite3_column_double(stmt, i);
              break;

            case SQLITE_TEXT: {
                // It shouldn't be necessary to copy or free() this value,
                // according to http://www.sqlite.org/c3ref/column_blob.html
                // it will be reclaimed on the next step, reset, or finalize.
                // I'm going to assume it's okay to keep this pointer around
                // until it is used in `EIO_AfterStep`
                sto->column_data_[i] = (char *) sqlite3_column_text(stmt, i);
              }
              break;

            default: {
              assert(0 && "unsupported type");
            }
          }
          assert(sto->column_data_[i]);
          assert(sto->column_names_[i]);
          assert(sto->column_types_[i]);
        }
        assert(sto->column_data_);
        assert(sto->column_names_);
        assert(sto->column_types_);
      }
      else if (rc == SQLITE_DONE) {
        // nothing to do in this case
      }
      else {
        sto->error_ = true;
      }

      return 0;
    }

    static Handle<Value> Step(const Arguments& args) {
      HandleScope scope;

      Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

      if (sto->HasCallback()) {
        return ThrowException(Exception::Error(String::New("Already stepping")));
      }

      REQ_FUN_ARG(0, cb);

      sto->SetCallback(cb);

      eio_custom(EIO_Step, EIO_PRI_DEFAULT, EIO_AfterStep, sto);

      ev_ref(EV_DEFAULT_UC);

      return Undefined();
    }

    // The following three methods must be called inside a HandleScope

    bool HasCallback() {
      return ! handle_->GetHiddenValue(callback_sym).IsEmpty();
    }

    void SetCallback(Local<Function> cb) {
      handle_->SetHiddenValue(callback_sym, cb);
    }

    Local<Function> GetCallback() {
      Local<Value> cb_v = handle_->GetHiddenValue(callback_sym);
      assert(cb_v->IsFunction());
      Local<Function> cb = Local<Function>::Cast(cb_v);
      handle_->DeleteHiddenValue(callback_sym);
      return cb;
    }

    int  column_count_;
    int  *column_types_;
    char **column_names_;
    void **column_data_;
    bool error_;

    int first_rc_;
    sqlite3_stmt* stmt_;
  };
};

Persistent<FunctionTemplate> Sqlite3Db::Statement::constructor_template;
Persistent<FunctionTemplate> Sqlite3Db::constructor_template;

extern "C" void init (v8::Handle<Object> target) {
  Sqlite3Db::Init(target);
}

