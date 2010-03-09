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

#include <v8.h>
#include <node.h>
#include <node_events.h>

#include <sqlite3.h>

using namespace v8;
using namespace node;

#define CHECK(rc) { if ((rc) != SQLITE_OK)                              \
      return ThrowException(Exception::Error(String::New(               \
                                             sqlite3_errmsg(*db)))); }

#define SCHECK(rc) { if ((rc) != SQLITE_OK)                             \
      return ThrowException(Exception::Error(String::New(               \
                        sqlite3_errmsg(sqlite3_db_handle(*stmt))))); }

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
  Local<Function> cb = Local<Function>::Cast(args[I]);

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

class Sqlite3Db : public EventEmitter {
public:
  static void Init(v8::Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    t->Inherit(EventEmitter::constructor_template);
    t->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(t, "open", Open);
    NODE_SET_PROTOTYPE_METHOD(t, "printIt", PrintIt);
    NODE_SET_PROTOTYPE_METHOD(t, "changes", Changes);
    NODE_SET_PROTOTYPE_METHOD(t, "close", Close);
    NODE_SET_PROTOTYPE_METHOD(t, "lastInsertRowid", LastInsertRowid);
    NODE_SET_PROTOTYPE_METHOD(t, "prepare", Prepare);

    target->Set(v8::String::NewSymbol("Database"), t->GetFunction());

    Statement::Init(target);
  }

protected:
  Sqlite3Db() : db_(NULL) { }

  ~Sqlite3Db() {
    sqlite3_close(db_);
  }

  sqlite3* db_;

  operator sqlite3* () const { return db_; }

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

    open_req->cb->Call(Context::GetCurrent()->Global(), err ? 1 : 0, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    open_req->cb.Dispose();
    free(open_req);

    open_req->dbo->Unref();

    return 0;
  }

  static int EIO_Open(eio_req *req) {
    struct open_request *open_req = (struct open_request *)(req->data);

    sqlite3 **dbptr = open_req->dbo->GetDBPtr();
    int rc = sqlite3_open(open_req->filename, dbptr);
    req->result = rc;

    sqlite3 *db = *dbptr;
    sqlite3_commit_hook(db, CommitHook, open_req->dbo);
    sqlite3_rollback_hook(db, RollbackHook, open_req->dbo);
    sqlite3_update_hook(db, UpdateHook, open_req->dbo);

    return 0;
  }

  static Handle<Value> PrintIt(const Arguments& args) {
    HandleScope scope;

    Sqlite3Db* dbo = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    sqlite3 *db = *dbo;

    return Undefined();
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
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    Local<Number> result = Integer::New(sqlite3_changes(*db));
    return scope.Close(result);
  }

  // TODO: libeio'fy
  static Handle<Value> Close(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    CHECK(sqlite3_close(*db));
    db->db_ = NULL;
    return Undefined();
  }

  // TODO: libeio'fy
  static Handle<Value> LastInsertRowid(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    Local<Number> result = Integer::New(sqlite3_last_insert_rowid(*db));
    return scope.Close(result);
  }

  // Hooks

  static int CommitHook(void* v_this) {
    HandleScope scope;
    Sqlite3Db* db = static_cast<Sqlite3Db*>(v_this);
    db->Emit(String::New("commit"), 0, NULL);
    // TODO: allow change in return value to convert to rollback...somehow
    return 0;
  }

  static void RollbackHook(void* v_this) {
    HandleScope scope;
    Sqlite3Db* db = static_cast<Sqlite3Db*>(v_this);
    db->Emit(String::New("rollback"), 0, NULL);
  }

  static void UpdateHook(void* v_this, int operation, const char* database,
                         const char* table, sqlite_int64 rowid) {
    HandleScope scope;
    Sqlite3Db* db = static_cast<Sqlite3Db*>(v_this);
    Local<Value> args[] = { Int32::New(operation), String::New(database),
                            String::New(table), Number::New(rowid) };
    db->Emit(String::New("update"), 4, args);
  }

  struct prepare_request {
    Persistent<Function> cb;
    Sqlite3Db *dbo;
    sqlite3_stmt* stmt;
    const char* tail;
    char sql[1];
  };

  static int EIO_AfterPrepare(eio_req *req) {
    ev_unref(EV_DEFAULT_UC);
    struct prepare_request *prep_req = (struct prepare_request *)(req->data);
    HandleScope scope;

    Local<Value> argv[2];
    bool err = false;
    if (req->result) {
      err = true;
      argv[0] = Exception::Error(String::New("Error preparing statement"));
    }
    else {
      Local<Value> arg = External::New(prep_req->stmt);
      Persistent<Object> statement(Statement::constructor_template->
                                   GetFunction()->NewInstance(1, &arg));
      if (prep_req->tail)
        statement->Set(String::New("tail"), String::New(prep_req->tail));

      argv[0] = Local<Value>::New(Undefined());
      argv[1] = scope.Close(statement);
    }

    TryCatch try_catch;

    prep_req->cb->Call(Context::GetCurrent()->Global(), (err ? 1 : 2), argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    prep_req->cb.Dispose();
    free(prep_req);

    prep_req->dbo->Unref();

    return 0;
  }

  static int EIO_Prepare(eio_req *req) {
    struct prepare_request *prep_req = (struct prepare_request *)(req->data);

    prep_req->stmt = NULL;
    prep_req->tail = NULL;
    sqlite3* db = *(prep_req->dbo);

    int rc = sqlite3_prepare_v2(db, prep_req->sql, -1,
                &(prep_req->stmt), &(prep_req->tail));

    req->result = rc;

    return 0;
  }

  static Handle<Value> Prepare(const Arguments& args) {
    HandleScope scope;
    REQ_STR_ARG(0, sql);
    REQ_FUN_ARG(1, cb);

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

      t->Inherit(EventEmitter::constructor_template);
      t->InstanceTemplate()->SetInternalFieldCount(1);

      NODE_SET_PROTOTYPE_METHOD(t, "bind", Bind);
      NODE_SET_PROTOTYPE_METHOD(t, "clearBindings", ClearBindings);
      NODE_SET_PROTOTYPE_METHOD(t, "finalize", Finalize);
      NODE_SET_PROTOTYPE_METHOD(t, "bindParameterCount", BindParameterCount);
      NODE_SET_PROTOTYPE_METHOD(t, "reset", Reset);
      NODE_SET_PROTOTYPE_METHOD(t, "step", Step);

      //target->Set(v8::String::NewSymbol("SQLStatement"), t->GetFunction());
    }

    static Handle<Value> New(const Arguments& args) {
      HandleScope scope;
      int I = 0;
      REQ_EXT_ARG(0, stmt);
      (new Statement((sqlite3_stmt*)stmt->Value()))->Wrap(args.This());
      return args.This();
    }

  protected:
    Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}

    ~Statement() { if (stmt_) sqlite3_finalize(stmt_); }

    sqlite3_stmt* stmt_;

    operator sqlite3_stmt* () const { return stmt_; }

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

      Statement *sto = bind_req->sto;

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
      sto->Unref();

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
          printf("key was %d\n", index);
          break;

        case KEY_STRING:
          index = sqlite3_bind_parameter_index(
                      *sto, (char*)(bind_req->key));
          printf("key was %s (index %d)\n", (char*)(bind_req->key), index);
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
          printf("value was an int\n");
          rc = sqlite3_bind_int(*sto, index, *(int*)(bind_req->value));
          break;
        case VALUE_DOUBLE:
          printf("value was a double %f\n", *(double*)(bind_req->value));
          rc = sqlite3_bind_double(*sto, index, *(double*)(bind_req->value));
          break;
        case VALUE_STRING:
          printf("value was a string, '%s'\n", (char*)(bind_req->value));
          rc = sqlite3_bind_text(*sto, index, (char*)(bind_req->value),
                            bind_req->value_size, SQLITE_TRANSIENT);
          break;
        case VALUE_NULL:
          printf("value was NULL\n");
          rc = sqlite3_bind_null(*sto, index);
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
         printf("attempting to bind to key %s\n", key);
      }
      else if (args[0]->IsInt32()) {
        bind_req->key_type = KEY_INT;
        int *index = (int *) malloc(sizeof(int));
        *index = args[0]->Int32Value();
        bind_req->value_size = sizeof(int);

        // don't forget to `free` this
        bind_req->key = index;
        printf("attempting to bind to index %d\n", (int) *index);
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
      sto->Ref();

      return Undefined();
    }

    // XXX TODO
    static Handle<Value> BindParameterCount(const Arguments& args) {
      HandleScope scope;
      Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());
      Local<Number> result = Integer::New(sqlite3_bind_parameter_count(*stmt));
      return scope.Close(result);
    }

    static Handle<Value> ClearBindings(const Arguments& args) {
      HandleScope scope;
      Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());
      SCHECK(sqlite3_clear_bindings(*stmt));
      return Undefined();
    }

    static Handle<Value> Finalize(const Arguments& args) {
      HandleScope scope;
      Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());
      SCHECK(sqlite3_finalize(*stmt));
      stmt->stmt_ = NULL;
      //args.This().MakeWeak();
      return Undefined();
    }

    static Handle<Value> Reset(const Arguments& args) {
      HandleScope scope;
      Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());
      SCHECK(sqlite3_reset(*stmt));
      return Undefined();
    }

    struct step_request {
      Persistent<Function> cb;
      Statement *sto;

      // Populate two arrays: one will be the types of columns for the result,
      // the second will be the data for the row.
      int  column_count;
      int  *column_types;
      void **column_data;
    };

    static int EIO_AfterStep(eio_req *req) {
      ev_unref(EV_DEFAULT_UC);

      HandleScope scope;

      struct step_request *step_req = (struct step_request *)(req->data);
      void **data = step_req->column_data;

      printf("there were %d columns\n", step_req->column_count);
      Local<Value> argv[2];
      argv[0] = Local<Value>::New(Undefined());
      Local<Array> row = Array::New(step_req->column_count);

      for (int i = 0; i < step_req->column_count; i++) {
        switch(step_req->column_types[i]) {
          case SQLITE_INTEGER:
            printf("integer value was %d\n", *(int*)(step_req->column_data[i]));
            row->Set(Number::New(i), Int32::New(*(int*) (step_req->column_data[i])));
            free((int*)(step_req->column_data[i]));
            break;
          case SQLITE_TEXT:
            printf("string value was %s\n", (char *) (step_req->column_data[i]));
            row->Set(Number::New(i),
                String::New((char *) (step_req->column_data[i])));
            // don't free this pointer, it's owned by sqlite3
            break;
          default:
            printf("Unknown type error!\n");
        }
      }

      argv[1] = row;
      TryCatch try_catch;

      step_req->cb->Call(Context::GetCurrent()->Global(), 2, argv);

      if (try_catch.HasCaught()) {
        FatalException(try_catch);
      }

      step_req->cb.Dispose();
      free(step_req->column_data);
      free(step_req->column_types);
      free(step_req);

      step_req->sto->Unref();
      return 0;
    }


    static int EIO_Step(eio_req *req) {
      struct step_request *step_req = (struct step_request *)(req->data);
      Statement *sto = step_req->sto;

//       int rc = sqlite3_step(*stmt);
//       if (rc == SQLITE_ROW) {
//         Local<Object> row = Object::New();
//         for (int c = 0; c < sqlite3_column_count(*stmt); ++c) {
//           Handle<Value> value;
//           switch (sqlite3_column_type(*stmt, c)) {
//           case SQLITE_INTEGER:
//             value = Integer::New(sqlite3_column_int(*stmt, c));
//             break;
//           case SQLITE_FLOAT:
//             value = Number::New(sqlite3_column_double(*stmt, c));
//             break;
//           case SQLITE_TEXT:
//             value = String::New((const char*) sqlite3_column_text(*stmt, c));
//             break;
//           case SQLITE_NULL:
//           default: // We don't handle any other types just now
//             value = Undefined();
//             break;
//           }
//           row->Set(String::NewSymbol(sqlite3_column_name(*stmt, c)),
//                    value);
//         }
//         return row;
//       } else if (rc == SQLITE_DONE) {
//         return Null();
//       } else {
//         return ThrowException(Exception::Error(String::New(
//                             sqlite3_errmsg(sqlite3_db_handle(*stmt)))));
//       }

      sqlite3_stmt *stmt = *sto;
      int rc = sqlite3_step(stmt);

      printf("result of step %d\n", rc);
      if (rc == SQLITE_ROW) {
        // would be nice to cache the column type data somewhere
        step_req->column_count = sqlite3_column_count(stmt);
        step_req->column_types =
          (int *) calloc(step_req->column_count, sizeof(int));
        step_req->column_data =
          (void **) calloc(step_req->column_count, sizeof(void *));

        for (int i = 0; i < step_req->column_count; i++) {
          int type = step_req->column_types[i] = sqlite3_column_type(stmt, i);

          switch(type) {
            case SQLITE_INTEGER: {
                int *value = (int *) malloc(sizeof(int));
                *value = sqlite3_column_int(stmt, i);
                step_req->column_data[i] = value;
                printf("serialized int %d\n", *(int*)(step_req->column_data[i]));
              }
              break;

            case SQLITE_FLOAT: {
                double *value = (double *) malloc(sizeof(double));
                *value = sqlite3_column_double(stmt, i);
                step_req->column_data[i] = value;
                printf("serialized float\n");
              }
              break;

            case SQLITE_TEXT: {
                // It shouldn't be necessary to copy or free() this value,
                // according to http://www.sqlite.org/c3ref/column_blob.html
                // it will be reclaimed on the next step, reset, or finalize.
                // I'm going to assume it's okay to keep this pointer around
                // until it is used in `EIO_AfterStep`
                char *value = (char *) sqlite3_column_text(stmt, i);
                step_req->column_data[i] = value;
                printf("serialized text %s\n", step_req->column_data[i]);
              }
              break;

            default: {
              // unsupported type
              printf("default, not serialized\n");
            }
          }
        }
      }
      else if (rc == SQLITE_DONE) {
        // done case
      }
      else {
        // error case
      }

      req->result = 0;
      return 0;
    }

    static Handle<Value> Step(const Arguments& args) {
      HandleScope scope;

      REQ_FUN_ARG(0, cb);

      Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

      struct step_request *step_req = (struct step_request *)
          calloc(1, sizeof(struct step_request));

      if (!step_req) {
        V8::LowMemoryNotification();
        return ThrowException(Exception::Error(
          String::New("Could not allocate enough memory")));
      }

      step_req->cb = Persistent<Function>::New(cb);
      step_req->sto = sto;

      eio_custom(EIO_Step, EIO_PRI_DEFAULT, EIO_AfterStep, step_req);

      ev_ref(EV_DEFAULT_UC);
      sto->Ref();

      return Undefined();
    }

    static Handle<Value> Step2(const Arguments& args) {
      HandleScope scope;
      Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());
      int rc = sqlite3_step(*stmt);
      if (rc == SQLITE_ROW) {
        Local<Object> row = Object::New();
        for (int c = 0; c < sqlite3_column_count(*stmt); ++c) {
          Handle<Value> value;
          switch (sqlite3_column_type(*stmt, c)) {
          case SQLITE_INTEGER:
            value = Integer::New(sqlite3_column_int(*stmt, c));
            break;
          case SQLITE_FLOAT:
            value = Number::New(sqlite3_column_double(*stmt, c));
            break;
          case SQLITE_TEXT:
            value = String::New((const char*) sqlite3_column_text(*stmt, c));
            break;

          default: // We don't handle any other types just now
            value = Undefined();
            break;
          }
          row->Set(String::NewSymbol(sqlite3_column_name(*stmt, c)),
                   value);
        }
        return row;
      } else if (rc == SQLITE_DONE) {
        return Null();
      } else {
        return ThrowException(Exception::Error(String::New(
                            sqlite3_errmsg(sqlite3_db_handle(*stmt)))));
      }
    }

    /*
    Handle<Object> Cast() {
      HandleScope scope;
      Local<ObjectTemplate> t(ObjectTemplate::New());
      t->SetInternalFieldCount(1);
      Local<Object> thus = t->NewInstance();
      thus->SetInternalField(0, External::New(this));
      //Wrap(thus);
      return thus;
    }
    */

  };

};


Persistent<FunctionTemplate> Sqlite3Db::Statement::constructor_template;

extern "C" void init (v8::Handle<Object> target) {
  Sqlite3Db::Init(target);
}

