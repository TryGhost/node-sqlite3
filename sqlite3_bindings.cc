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
#include <deque>

using namespace v8;
using namespace node;

class Sqlite3Db : public EventEmitter
{
public:
  static void Init(v8::Handle<Object> target) 
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    t->Inherit(EventEmitter::constructor_template);
    t->InstanceTemplate()->SetInternalFieldCount(1);
  
    NODE_SET_PROTOTYPE_METHOD(t, "performQuery", PerformQuery);
    NODE_SET_PROTOTYPE_METHOD(t, "close", Close);
  
    target->Set(v8::String::NewSymbol("DatabaseSync"), t->GetFunction());
  }

protected:
  Sqlite3Db(sqlite3* db) : db_(db) {
  }

  ~Sqlite3Db() {
    sqlite3_close(db_);
  }

  sqlite3* db_;

  operator sqlite3* () { return db_; }

protected:
  static Handle<Value> New(const Arguments& args)
  {
    HandleScope scope;

    if (args.Length() == 0 || !args[0]->IsString()) {
      return ThrowException(Exception::TypeError(
            String::New("First argument must be a string")));
    }

    String::Utf8Value filename(args[0]->ToString());
    sqlite3* db;
    int rc = sqlite3_open(*filename, &db);
    if (rc) {
      Local<String> err = v8::String::New(sqlite3_errmsg(db));
      sqlite3_close(db);
      return ThrowException(Exception::Error(err));
    }
    (new Sqlite3Db(db))->Wrap(args.This());
    return args.This();
  }


  static Handle<Value> PerformQuery(const Arguments& args)
  {
    HandleScope scope;
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());

    if (args.Length() == 0 || !args[0]->IsString()) {
      return ThrowException(Exception::TypeError(
            String::New("First argument must be a string")));
    }

    String::Utf8Value usql(args[0]->ToString());
    const char* sql(*usql);

    int changes = 0;
    int param = 0;

    std::deque< Handle<Array> > resulting;

    for(;;) {

      sqlite3_stmt* stmt;
      int rc = sqlite3_prepare_v2(*db, sql, -1, &stmt, &sql);
      if (!stmt) break;
      Statement statement(stmt);
 
      if (args.Length() > 1) {
        if (args[1]->IsArray()) {
          Local<Array> a(Array::Cast(*args[1]));
          int start = param;
          int stop = start + sqlite3_bind_parameter_count(statement);
          for (; param < a->Length() && param < stop; ++param) {
            Local<Value> v = a->Get(Integer::New(param));
            statement.Bind(param+1-start, v);
          }
        } else if (args[1]->IsObject()) {
          Local<Array> keys(args[1]->ToObject()->GetPropertyNames());
          for (int k = 0; k < keys->Length(); ++k) {
            Local<Value> key(keys->Get(Integer::New(k)));
            statement.Bind(key, args[1]->ToObject()->Get(key));
          }
        } else if (args[1]->IsUndefined() || args[1]->IsNull()) {
          // That's okay
        } else {
          return ThrowException(Exception::TypeError(
                         String::New("Second argument invalid")));
        }
      }
      
      std::deque< Handle<Object> > rows;

      for (int r = 0; ; ++r) {
        int rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) {
          Local<Object> row = Object::New();
          for (int c = 0; c < sqlite3_column_count(statement); ++c) {
            Handle<Value> value;
            switch (sqlite3_column_type(statement, c)) {
            case SQLITE_INTEGER:
              value = Integer::New(sqlite3_column_int(statement, c));
              break;
            case SQLITE_FLOAT: 
              value = Number::New(sqlite3_column_double(statement, c));
              break;
            case SQLITE_TEXT: 
              value = String::New((const char*) sqlite3_column_text(statement, c));
              break;
            case SQLITE_NULL:
            default: // We don't handle any other types just now
              value = Undefined();
              break;
            }
            row->Set(String::NewSymbol(sqlite3_column_name(statement, c)), 
                     value);
          }
          rows.push_back(row);
        } else if (rc == SQLITE_DONE) {
          break;
        } else {
          return ThrowException(Exception::Error(
                                      v8::String::New(sqlite3_errmsg(*db))));
        }
      }

      changes += sqlite3_changes(*db);

      Local<Array> rosult(Array::New(rows.size()));
      std::deque< Handle<Object> >::const_iterator ri(rows.begin());
      for (int r = 0; r < rows.size(); ++r, ++ri)
        rosult->Set(Integer::New(r), *ri);
      rosult->Set(String::New("rowsAffected"), Integer::New(sqlite3_changes(*db)));
      rosult->Set(String::New("insertId"), 
                  Integer::New(sqlite3_last_insert_rowid(*db)));
      resulting.push_back(rosult);
    }
      
    Local<Array> result(Array::New(0));
    result->Set(String::New("rowsAffected"), Integer::New(changes)); 
    result->Set(String::New("insertId"), 
                Integer::New(sqlite3_last_insert_rowid(*db)));
    std::deque< Handle<Array> >::iterator ri(resulting.begin());
    for (int r = 0; r < resulting.size(); ++r, ++ri) {
      result->Set(Integer::New(r), *ri);
    }
    return result;
  }


  static Handle<Value> Close (const Arguments& args)
  {
    Sqlite3Db* db = ObjectWrap::Unwrap<Sqlite3Db>(args.This());
    HandleScope scope;
    db->Close();
    return Undefined();
  }

  void Close() {
    sqlite3_close(db_);
    db_ = NULL;
    Detach();
  }

  class Statement : public EventEmitter
  {
  public:
    Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}
    
    ~Statement() { sqlite3_finalize(stmt_); }
    
    operator sqlite3_stmt* () { return stmt_; }
    
    bool Bind(int index, Handle<Value> value) 
    {
      HandleScope scope;
      if (value->IsInt32()) {
        sqlite3_bind_int(stmt_, index, value->Int32Value());
      } else if (value->IsNumber()) {
        sqlite3_bind_double(stmt_, index, value->NumberValue());
      } else if (value->IsString()) {
        String::Utf8Value text(value);
        sqlite3_bind_text(stmt_, index, *text, text.length(), SQLITE_TRANSIENT);
      } else {
        return false;
      }
      return true;
    }
    
    bool Bind(Handle<Value> key, Handle<Value> value) {
      HandleScope scope;
      String::Utf8Value skey(key);
      //string x = ":" + key
      int index = sqlite3_bind_parameter_index(stmt_, *skey);
      Bind(index, value);
    }
    
    Handle<Object> Cast()
    {
      HandleScope scope;
      Local<ObjectTemplate> t(ObjectTemplate::New());
      t->SetInternalFieldCount(1);
      Local<Object> thus = t->NewInstance();
      thus->SetInternalField(0, External::New(this));
      //Wrap(thus);
      return thus;
    }
    
  protected:
    sqlite3_stmt* stmt_;
  };

  
};


extern "C" void init (v8::Handle<Object> target)
{
  Sqlite3Db::Init(target);
}

