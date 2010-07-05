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

#ifndef STATEMENT_H
#define STATEMENT_H

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <sqlite3.h>

using namespace v8;
using namespace node;

class Statement : public EventEmitter {
  public:
  
    static Persistent<FunctionTemplate> constructor_template;
    
    static void Init(v8::Handle<Object> target);
    static Handle<Value> New(const Arguments& args);
  
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
    
    static int EIO_AfterBind(eio_req *req);
    static int EIO_Bind(eio_req *req);
    static Handle<Value> Bind(const Arguments& args);

    static int EIO_AfterBindArray(eio_req *req);
    static int EIO_BindArray(eio_req *req);
    static Handle<Value> BindArray(const Arguments& args);
    
    static int EIO_AfterFinalize(eio_req *req);
    static int EIO_Finalize(eio_req *req);
    static Handle<Value> Finalize(const Arguments& args);
    
    static Handle<Value> Reset(const Arguments& args);
    static Handle<Value> ClearBindings(const Arguments& args);
    
    static int EIO_AfterStep(eio_req *req);
    static int EIO_Step(eio_req *req);
    static Handle<Value> Step(const Arguments& args);
    void FreeColumnData(void);
    
    bool HasCallback();
    void SetCallback(Local<Function> cb);
    
    Local<Function> GetCallback();
  
  private:

    int  column_count_;
    int  *column_types_;
    char **column_names_;
    void **column_data_;
    bool error_;
    
    int first_rc_;
    sqlite3_stmt* stmt_;
};

// indicates the key type (integer index or name string)
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

  struct bind_pair *pairs;
  size_t len;
};

struct bind_pair {
  enum BindKeyType   key_type;
  enum BindValueType value_type;

  void *key;   // char * | int *
  void *value; // char * | int * | double * | 0
  size_t value_size;
};

#endif
