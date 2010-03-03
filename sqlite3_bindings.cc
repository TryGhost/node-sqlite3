/*
Copyright (c) 2009, Eric Fredricksen <e@fredricksen.net>

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

class Sqlite3Db : public EventEmitter
{
public:
  static void Init(v8::Handle<Object> target)
  {
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

    printf("EIO_AfterOpen; rc = %d\n", (int) req->result);
    printf("result was %d\n", (int) req->result);

    Local<Value> argv[1];
    bool err = false;
    if (req->result) {
      err = true;
      argv[0] = Exception::Error(String::New("Error opening database"));
    }

    TryCatch try_catch;

    open_req->cb->Call(Context::GetCurrent()->Global(), err && 1, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    open_req->cb.Dispose();
    free(open_req);

    return 0;
  }

  static int EIO_Open(eio_req *req) {
    struct open_request *open_req = (struct open_request *)(req->data);
    printf("The filename was %s\n", open_req->filename);

    sqlite3 **dbptr = open_req->dbo->GetDBPtr();
    printf("before assn %p\n", *dbptr);
    int rc = sqlite3_open(open_req->filename, dbptr);
    printf("after assn %p\n", *dbptr);

    sqlite3 *db = *dbptr;
    sqlite3_commit_hook(db, CommitHook, open_req->dbo);
    sqlite3_rollback_hook(db, RollbackHook, open_req->dbo);
    sqlite3_update_hook(db, UpdateHook, open_req->dbo);
    req->result = rc;
    return 0;
  }

  static Handle<Value> PrintIt(const Arguments& args) {
    HandleScope scope;

    Sqlite3Db* dbo = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    sqlite3 *db = *dbo;

    printf("print it %p\n", db);

    return Undefined();
  }

  static Handle<Value> Open(const Arguments& args) {
    HandleScope scope;

    REQ_STR_ARG(0, filename);
    REQ_FUN_ARG(1, cb);

    Sqlite3Db* dbo = ObjectWrap::Unwrap<Sqlite3Db>(args.This());

    printf("way before addr %p\n", ((sqlite3*) *dbo));

    struct open_request *open_req = (struct open_request *)
        calloc(1, sizeof(struct open_request));

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

  static Handle<Value> Changes(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    Local<Number> result = Integer::New(sqlite3_changes(*db));
    return scope.Close(result);
  }

  static Handle<Value> Close(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    CHECK(sqlite3_close(*db));
    db->db_ = NULL;
    return Undefined();
  }

  static Handle<Value> LastInsertRowid(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    Local<Number> result = Integer::New(sqlite3_last_insert_rowid(*db));
    return scope.Close(result);
  }

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

  /*
  static Handle<Value> Open(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    REQ_STR_ARG(0, filename);
    Close(args);  // ignores args anyway, except This
    CHECK(sqlite3_open(*filename, &db->db_));

    sqlite3_commit_hook(*db, CommitHook, db);
    sqlite3_rollback_hook(*db, RollbackHook, db);
    sqlite3_update_hook(*db, UpdateHook, db);

    return args.This();
  }
  */

  static Handle<Value> Prepare(const Arguments& args) {
    HandleScope scope;
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    REQ_STR_ARG(0, sql);
    sqlite3_stmt* stmt = NULL;
    const char* tail = NULL;
    CHECK(sqlite3_prepare_v2(*db, *sql, -1, &stmt, &tail));
    if (!stmt)
      return Null();
    Local<Value> arg = External::New(stmt);
    Persistent<Object> statement(Statement::constructor_template->
                                 GetFunction()->NewInstance(1, &arg));
    if (tail)
      statement->Set(String::New("tail"), String::New(tail));
    return scope.Close(statement);
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

    static Handle<Value> Bind(const Arguments& args) {
      HandleScope scope;
      Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

      REQ_ARGS(2);
      if (!args[0]->IsString() && !args[0]->IsInt32())
        return ThrowException(Exception::TypeError(
               String::New("First argument must be a string or integer")));
      int index = args[0]->IsString() ?
        sqlite3_bind_parameter_index(*stmt, *String::Utf8Value(args[0])) :
        args[0]->Int32Value();

      if (args[1]->IsInt32()) {
        sqlite3_bind_int(*stmt, index, args[1]->Int32Value());
      } else if (args[1]->IsNumber()) {
        sqlite3_bind_double(*stmt, index, args[1]->NumberValue());
      } else if (args[1]->IsString()) {
        String::Utf8Value text(args[1]);
        sqlite3_bind_text(*stmt, index, *text, text.length(),SQLITE_TRANSIENT);
      } else if (args[1]->IsNull() || args[1]->IsUndefined()) {
        sqlite3_bind_null(*stmt, index);
      } else {
        return ThrowException(Exception::TypeError(
               String::New("Unable to bind value of this type")));
      }
      return args.This();
    }

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

    static Handle<Value> Step(const Arguments& args) {
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
          case SQLITE_NULL:
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

