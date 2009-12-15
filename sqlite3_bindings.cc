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
#include <sqlite3.h>
#include <v8.h>
#include <node.h>
#include <node_events.h>

using namespace v8;
using namespace node;


#define CHECK(rc) { if ((rc) != SQLITE_OK)                              \
      return ThrowException(Exception::Error(String::New(               \
                                             sqlite3_errmsg(*db)))); }

#define SCHECK(rc) { if ((rc) != SQLITE_OK) \
      return ThrowException(Exception::Error(String::New(               \
                        sqlite3_errmsg(sqlite3_db_handle(*stmt))))); }

#define REQ_ARGS(N)                                                     \
  if (args.Length() < (N))                                              \
    return ThrowException(Exception::TypeError(                         \
                             String::New("Expected " #N "arguments"))); 

#define REQ_STR_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsString())                     \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a string"))); \
  String::Utf8Value VAR(args[I]->ToString());
                                                      
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
              String::New("Argument " #I " must be an integer"))); \
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
    
    NODE_SET_PROTOTYPE_METHOD(t, "changes", Changes);
    NODE_SET_PROTOTYPE_METHOD(t, "close", Close);
    NODE_SET_PROTOTYPE_METHOD(t, "lastInsertRowid", LastInsertRowid);
    NODE_SET_PROTOTYPE_METHOD(t, "prepare", Prepare);
    
    target->Set(v8::String::NewSymbol("DatabaseSync"), t->GetFunction());

    Statement::Init(target);
  }

protected:
  Sqlite3Db(sqlite3* db) : db_(db) {
  }

  ~Sqlite3Db() {
    sqlite3_close(db_);
  }

  sqlite3* db_;

  operator sqlite3* () const { return db_; }

protected:
  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;
    REQ_STR_ARG(0, filename);
    sqlite3* db;
    int rc = sqlite3_open(*filename, &db);
    if (rc) return ThrowException(Exception::Error(                     
                                     String::New("Error opening database")));
    Sqlite3Db* dbo = new Sqlite3Db(db);
    dbo->Wrap(args.This());

    sqlite3_commit_hook(db, CommitHook, dbo);
    sqlite3_rollback_hook(db, RollbackHook, dbo);
    sqlite3_update_hook(db, UpdateHook, dbo);

    return args.This();
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
    db->Emit("commit", 0, NULL); 
    // TODO: allow change in return value to convert to rollback...somehow
    return 0;
  }

  static void RollbackHook(void* v_this) {
    HandleScope scope;
    Sqlite3Db* db = static_cast<Sqlite3Db*>(v_this);
    db->Emit("rollback", 0, NULL); 
  }

  static void UpdateHook(void* v_this, int operation, const char* database, 
                         const char* table, sqlite_int64 rowid) {
    HandleScope scope;
    Sqlite3Db* db = static_cast<Sqlite3Db*>(v_this);
    Local<Value> args[] = { Int32::New(operation), String::New(database),
                            String::New(table), Number::New(rowid) };
    db->Emit("update", 4, args);
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

  class Statement : public EventEmitter
  {
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


extern "C" void init (v8::Handle<Object> target)
{
  Sqlite3Db::Init(target);
}

