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
#include <stdlib.h>
#include <node_buffer.h>

extern "C" {
  #include <mpool.h>
};

using namespace v8;
using namespace node;

struct cell_node {
  void *value;
  int type;
  struct cell_node *next;
  int size;
};

struct row_node {
  struct cell_node *cells;
  struct row_node *next;
};


// represent strings with this struct
struct string_t {
  size_t bytes;
  char data[];
};

class Statement : public EventEmitter {
  public:

    static Persistent<FunctionTemplate> constructor_template;

    static void Init(v8::Handle<Object> target);
    static Handle<Value> New(const Arguments& args);

  protected:

    Statement(sqlite3_stmt* stmt, int first_rc = -1, int mode = 0)
    : EventEmitter(), first_rc_(first_rc), mode_(mode), stmt_(stmt) {
      column_count_ = -1;
      column_names_ = NULL;
    }

    ~Statement() {
      if (stmt_) sqlite3_finalize(stmt_);
      if (column_names_) FreeColumnData();
    }

    static Handle<Value> Bind(const Arguments &args);
    static Handle<Value> BindObject(const Arguments &args);
    static Handle<Value> BindArray(const Arguments &args);
    static int EIO_BindArray(eio_req *req);
    static int EIO_AfterBindArray(eio_req *req);

    static int EIO_AfterFinalize(eio_req *req);
    static int EIO_Finalize(eio_req *req);
    static Handle<Value> Finalize(const Arguments &args);

    static Handle<Value> Reset(const Arguments &args);
    static Handle<Value> ClearBindings(const Arguments &args);

    static int EIO_AfterStep(eio_req *req);
    static int EIO_Step(eio_req *req);
    static Handle<Value> Step(const Arguments &args);

    static int EIO_AfterFetchAll(eio_req *req);
    static int EIO_FetchAll(eio_req *req);
    static Handle<Value> FetchAll(const Arguments &args);

    void InitializeColumns(void);
    void FreeColumnData(void);

    bool HasCallback();
    void SetCallback(Local<Function> cb);

    Local<Function> GetCallback();

  private:

    int  column_count_;
    char **column_names_;
    int error_;
    Local<String> error_msg_;

    int first_rc_;
    int mode_;
    sqlite3_stmt* stmt_;

    // for statment.step
    cell_node *cells;
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
  VALUE_BLOB,
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

struct fetchall_request {
  Persistent<Function> cb;
  Statement *sto;
  mpool_t *pool;
  char *error;
  struct row_node *rows;
};

#endif
