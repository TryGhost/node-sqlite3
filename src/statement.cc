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

extern "C" {
  #include <mpool.h>
};

#include "database.h"
#include "statement.h"
#include "sqlite3_bindings.h"

static Persistent<String> callback_sym;
Persistent<FunctionTemplate> Statement::constructor_template;

void Statement::Init(v8::Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);

  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->Inherit(EventEmitter::constructor_template);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("Statement"));

  NODE_SET_PROTOTYPE_METHOD(t, "bind", Bind);
  NODE_SET_PROTOTYPE_METHOD(t, "bindObject", BindObject);
  NODE_SET_PROTOTYPE_METHOD(t, "bindArray", BindArray);

  NODE_SET_PROTOTYPE_METHOD(t, "finalize", Finalize);
  NODE_SET_PROTOTYPE_METHOD(t, "reset", Reset);
  NODE_SET_PROTOTYPE_METHOD(t, "clearBindings", ClearBindings);
  NODE_SET_PROTOTYPE_METHOD(t, "step", Step);
  NODE_SET_PROTOTYPE_METHOD(t, "fetchAll", FetchAll);

  callback_sym = Persistent<String>::New(String::New("callback"));
}

Handle<Value> Statement::New(const Arguments& args) {
  HandleScope scope;
  REQ_EXT_ARG(0, stmt);
  int first_rc = args[1]->IntegerValue();
  int mode = args[2]->IntegerValue();

  Statement *sto = new Statement((sqlite3_stmt*)stmt->Value(), first_rc, mode);
  sto->Wrap(args.This());
  sto->Ref();

  return args.This();
}

int Statement::EIO_AfterBindArray(eio_req *req) {
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

  struct bind_pair *pair = bind_req->pairs;

  for (size_t i = 0; i < bind_req->len; i++, pair++) {
    free((char*)(pair->key));
    switch(pair->key_type) {
      case KEY_INT:
        free((int*)(pair->value));
        break;

      case KEY_STRING:
        free((char*)(pair->value));
        break;
    }
  }
  free(bind_req->pairs);
  free(bind_req);

  return 0;
}

int Statement::EIO_BindArray(eio_req *req) {
  struct bind_request *bind_req = (struct bind_request *)(req->data);
  Statement *sto = bind_req->sto;
  int rc(0);

  struct bind_pair *pair = bind_req->pairs;

  int index = 0;
  for (size_t i = 0; i < bind_req->len; i++, pair++) {
    switch(pair->key_type) {
      case KEY_INT:
        index = *(int*)(pair->key);
        break;

      case KEY_STRING:
        index = sqlite3_bind_parameter_index(sto->stmt_,
            (char*)(pair->key));
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
    switch(pair->value_type) {
      case VALUE_INT:
        rc = sqlite3_bind_int(sto->stmt_, index, *(int*)(pair->value));
        break;
      case VALUE_DOUBLE:
        rc = sqlite3_bind_double(sto->stmt_, index, *(double*)(pair->value));
        break;
      case VALUE_STRING:
        rc = sqlite3_bind_text(sto->stmt_, index, (char*)(pair->value),
            pair->value_size, SQLITE_TRANSIENT);
        break;
      case VALUE_BLOB:
        rc = sqlite3_bind_blob(sto->stmt_, index, (char*)(pair->value),
          pair->value_size, SQLITE_TRANSIENT);
        break;
      case VALUE_NULL:
        rc = sqlite3_bind_null(sto->stmt_, index);
        break;

      // should be unreachable
    }
  }

  if (rc) {
    req->result = rc;
    return 0;
  }

  return 0;
}

Handle<Value> Statement::BindObject(const Arguments& args) {
  HandleScope scope;
  Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

  REQ_ARGS(2);
  REQ_FUN_ARG(1, cb);

  if (! args[0]->IsObject())
    return ThrowException(Exception::TypeError(
          String::New("First argument must be an Array.")));
  Local<Object> obj = args[0]->ToObject();
  Local<Array> properties = obj->GetPropertyNames();

  struct bind_request *bind_req = (struct bind_request *)
    calloc(1, sizeof(struct bind_request));

  int len = bind_req->len = properties->Length();
  bind_req->pairs = (struct bind_pair *)
    calloc(len, sizeof(struct bind_pair));

  struct bind_pair *pairs = bind_req->pairs;

  for (uint32_t i = 0; i < properties->Length(); i++, pairs++) {
    Local<Value> name = properties->Get(Integer::New(i));
    Local<Value> val = obj->Get(name->ToString());

    String::Utf8Value keyValue(name);

    // setting key type
    pairs->key_type = KEY_STRING;
    char *key = (char *) calloc(1, keyValue.length()+1);
    memcpy(key, *keyValue, keyValue.length()+1);
    pairs->key = key;

    // setup value
    if (val->IsInt32()) {
      pairs->value_type = VALUE_INT;
      int *value = (int *) malloc(sizeof(int));
      *value = val->Int32Value();
      pairs->value = value;
    }
    else if (val->IsNumber()) {
      pairs->value_type = VALUE_DOUBLE;
      double *value = (double *) malloc(sizeof(double));
      *value = val->NumberValue();
      pairs->value = value;
    }
    else if (val->IsString()) {
      pairs->value_type = VALUE_STRING;
      String::Utf8Value text(val);
      char *value = (char *) calloc(text.length(), sizeof(char));
      memcpy(value, *text, text.length());
      pairs->value = value;
      pairs->value_size = text.length();
    }
    else if (val->IsNull() || val->IsUndefined()) {
      pairs->value_type = VALUE_NULL;
      pairs->value = NULL;
    }
    else {
      free(pairs->key);
      return ThrowException(Exception::TypeError(
            String::New("Unable to bind value of this type")));
    }
  }

  bind_req->cb = Persistent<Function>::New(cb);
  bind_req->sto = sto;

  eio_custom(EIO_BindArray, EIO_PRI_DEFAULT, EIO_AfterBindArray, bind_req);

  ev_ref(EV_DEFAULT_UC);

  return Undefined();
};

Handle<Value> Statement::BindArray(const Arguments& args) {
  HandleScope scope;
  Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

  REQ_ARGS(2);
  REQ_FUN_ARG(1, cb);
  if (! args[0]->IsArray())
    return ThrowException(Exception::TypeError(
      String::New("First argument must be an Array.")));

  struct bind_request *bind_req = (struct bind_request *)
    calloc(1, sizeof(struct bind_request));

  Local<Array> array = Local<Array>::Cast(args[0]);
  int len = bind_req->len = array->Length();
  bind_req->pairs = (struct bind_pair *)
    calloc(len, sizeof(struct bind_pair));

  struct bind_pair *pairs = bind_req->pairs;

  // pack the binds into the struct
  for (int i = 0; i < len; i++, pairs++) {
    Local<Value> val = array->Get(i);

    // setting key type
    pairs->key_type = KEY_INT;
    int *index = (int *) malloc(sizeof(int));
    *index = i+1;

    // don't forget to `free` this
    pairs->key = index;

    // setup value
    if (val->IsInt32()) {
      pairs->value_type = VALUE_INT;
      int *value = (int *) malloc(sizeof(int));
      *value = val->Int32Value();
      pairs->value = value;
    }
    else if (val->IsNumber()) {
      pairs->value_type = VALUE_DOUBLE;
      double *value = (double *) malloc(sizeof(double));
      *value = val->NumberValue();
      pairs->value = value;
    }
    else if (val->IsString()) {
      pairs->value_type = VALUE_STRING;
      String::Utf8Value text(val);
      char *value = (char *) calloc(text.length(), sizeof(char));
      memcpy(value, *text, text.length());
      pairs->value = value;
      pairs->value_size = text.length();
    }
    else if (val->IsNull() || val->IsUndefined()) {
      pairs->value_type = VALUE_NULL;
      pairs->value = NULL;
    }
    else {
      free(pairs->key);
      return ThrowException(Exception::TypeError(
            String::New("Unable to bind value of this type")));
    }
  }

  bind_req->cb = Persistent<Function>::New(cb);
  bind_req->sto = sto;

  eio_custom(EIO_BindArray, EIO_PRI_DEFAULT, EIO_AfterBindArray, bind_req);

  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

// db.prepare "SELECT $x, ?" -> statement
//
// Bind multiple placeholders by array
//   statement.bind([ value0, value1 ], callback);
//
// Bind multiple placeholdders by name
//   statement.bind({ $x: value }, callback);
//
// Bind single placeholder by name:
//   statement.bind('$x', value, callback);
//
// Bind placeholder by position:
//   statement.bind(1, value, callback);

Handle<Value> Statement::Bind(const Arguments& args) {
  HandleScope scope;
  Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

  REQ_ARGS(2);
  REQ_FUN_ARG(2, cb);

  if (!(   args[0]->IsString()
        || args[0]->IsInt32()
        || args[0]->IsArray()
        || args[0]->IsObject()))
    return ThrowException(Exception::TypeError(
          String::New("First argument must be a string, number, array or object.")));

  struct bind_request *bind_req = (struct bind_request *)
    calloc(1, sizeof(struct bind_request));

  bind_req->len = 1;
  struct bind_pair *pair = bind_req->pairs = (struct bind_pair *)
    calloc(1, sizeof(struct bind_pair));

  // setup key
  if (args[0]->IsString()) {
    String::Utf8Value keyValue(args[0]);
    pair->key_type = KEY_STRING;

    char *key = (char *) calloc(1, keyValue.length()+1);
    memcpy(key, *keyValue, keyValue.length()+1);

    pair->key = key;
  }
  else if (args[0]->IsInt32()) {
    pair->key_type = KEY_INT;
    int *index = (int *) malloc(sizeof(int));
    *index = args[0]->Int32Value();

    // don't forget to `free` this
    pair->key = index;
  }

  // setup value
  if (args[1]->IsInt32()) {
    pair->value_type = VALUE_INT;
    int *value = (int *) malloc(sizeof(int));
    *value = args[1]->Int32Value();
    pair->value = value;
  }
  else if (args[1]->IsNumber()) {
    pair->value_type = VALUE_DOUBLE;
    double *value = (double *) malloc(sizeof(double));
    *value = args[1]->NumberValue();
    pair->value = value;
  }
  else if (args[1]->IsString()) {
    pair->value_type = VALUE_STRING;
    String::Utf8Value text(args[1]);
    char *value = (char *) calloc(text.length(), sizeof(char));
    memcpy(value, *text, text.length());
    pair->value = value;
    pair->value_size = text.length();
  }
  else if (Buffer::HasInstance(args[1])) {
    pair->value_type = VALUE_BLOB;
    Buffer* buffer = Buffer::Unwrap<Buffer>(args[1]->ToObject());
    pair->value = buffer->data();
    pair->value_size = buffer->length();
  }
  else if (args[1]->IsNull() || args[1]->IsUndefined()) {
    pair->value_type = VALUE_NULL;
    pair->value = NULL;
  }
  else {
    free(pair->key);
    return ThrowException(Exception::TypeError(
          String::New("Unable to bind value of this type")));
  }

  bind_req->cb = Persistent<Function>::New(cb);
  bind_req->sto = sto;

  eio_custom(EIO_BindArray, EIO_PRI_DEFAULT, EIO_AfterBindArray, bind_req);

  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

int Statement::EIO_AfterFinalize(eio_req *req) {
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

int Statement::EIO_Finalize(eio_req *req) {
  Statement *sto = (class Statement *)(req->data);

  assert(sto->stmt_);
  req->result = sqlite3_finalize(sto->stmt_);
  sto->stmt_ = NULL;

  return 0;
}

Handle<Value> Statement::Finalize(const Arguments& args) {
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

Handle<Value> Statement::ClearBindings(const Arguments& args) {
  HandleScope scope;
  Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());
  SCHECK(sqlite3_clear_bindings(sto->stmt_));
  return Undefined();
}

Handle<Value> Statement::Reset(const Arguments& args) {
  HandleScope scope;
  Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());
  sto->FreeColumnData();
  SCHECK(sqlite3_reset(sto->stmt_));
  return Undefined();
}

int Statement::EIO_AfterStep(eio_req *req) {
  ev_unref(EV_DEFAULT_UC);

  HandleScope scope;

  Statement *sto = (class Statement *) req->data;

  sqlite3* db = sqlite3_db_handle(sto->stmt_);
  Local<Value> argv[2];

  if (sto->error_) {
    argv[0] = Exception::Error(
        String::New(sqlite3_errmsg(db)));
  }
  else {
    argv[0] = Local<Value>::New(Undefined());
  }

  if (req->result == SQLITE_DONE) {
    argv[1] = Local<Value>::New(Null());
  }
  else {
    Local<Object> row = Object::New();

    struct cell_node *cell = sto->cells
                   , *next = NULL;

    for (int i = 0; cell; i++) {
      switch (cell->type) {
        case SQLITE_INTEGER:
          row->Set(String::NewSymbol((char*) sto->column_names_[i]),
                   Int32::New(*(int*) cell->value));
          free(cell->value);
          break;

        case SQLITE_FLOAT:
          row->Set(String::NewSymbol(sto->column_names_[i]),
                   Number::New(*(double*) cell->value));
          free(cell->value);
          break;

        case SQLITE_TEXT: {
            struct string_t *str = (struct string_t *) cell->value;

            // str->bytes-1 to compensate for the NULL terminator
            row->Set(String::NewSymbol(sto->column_names_[i]),
                     String::New(str->data, str->bytes-1));
            free(str);
          }
          break;

        case SQLITE_BLOB: {
            node::Buffer *buf = Buffer::New(cell->size);
            memcpy(buf->data(), cell->value, cell->size);
            row->Set(String::New(sto->column_names_[i]), buf->handle_);
            free(cell->value);
          }
          break;

        case SQLITE_NULL:
          row->Set(String::New(sto->column_names_[i]),
              Local<Value>::New(Null()));
          break;
      }
      next = cell->next;
      free(cell);
      cell=next;
    }
    argv[1] = row;
  }

  if (sto->mode_ & EXEC_LAST_INSERT_ID) {
    sto->handle_->Set(String::New("lastInsertRowID"),
      Integer::New(sqlite3_last_insert_rowid(db)));
  }

  if (sto->mode_ & EXEC_AFFECTED_ROWS) {
    sto->handle_->Set(String::New("affectedRows"),
      Integer::New(sqlite3_changes(db)));
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

void Statement::FreeColumnData(void) {
  if (!column_count_) return;
  free(column_names_);
  column_count_ = 0;
  column_names_ = NULL;
}

void Statement::InitializeColumns(void) {
  this->column_count_ = sqlite3_column_count(this->stmt_);
  this->column_names_ = (char **) calloc(this->column_count_, sizeof(char *));

  for (int i = 0; i < this->column_count_; i++) {
    // Don't free this!
    this->column_names_[i] = (char *) sqlite3_column_name(this->stmt_, i);
  }
}

int Statement::EIO_Step(eio_req *req) {
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
    // If this pointer is NULL, look up and store the columns names.
    if (!sto->column_names_) {
      sto->InitializeColumns();
    }

    struct cell_node *cell_head = NULL
                   , *cell_prev = NULL
                   , *cell      = NULL;

    for (int i = 0; i < sto->column_count_; i++) {
      cell = (struct cell_node *) malloc(sizeof(struct cell_node));

      // If this is the first cell, set `cell_head` to it, otherwise attach
      // the new cell to the end of the list `cell_prev->next`.
      (!cell_head ? cell_head : cell_prev->next) = cell;

      cell->type = sqlite3_column_type(sto->stmt_, i);
      cell->next = NULL;

      switch (cell->type) {
        case SQLITE_INTEGER:
          cell->value = (int *) malloc(sizeof(int));
          * (int *) cell->value = sqlite3_column_int(stmt, i);
          break;

        case SQLITE_FLOAT:
          cell->value = (double *) malloc(sizeof(double));
          * (double *) cell->value = sqlite3_column_double(stmt, i);
          break;

        case SQLITE_TEXT: {
            char *text = (char *) sqlite3_column_text(stmt, i);
            int size = 1+sqlite3_column_bytes(stmt, i);
            struct string_t *str = (struct string_t *)
                                     malloc(sizeof(size_t) + size);
            str->bytes = size;
            memcpy(str->data, text, size);
            cell->value = str;
          }
          break;

        case SQLITE_BLOB: {
            const void* blob = sqlite3_column_blob(stmt, i);
            int size = sqlite3_column_bytes(stmt, i);
            cell->size = size;
            unsigned char *pzBlob = (unsigned char *)malloc(size);
            memcpy(pzBlob, blob, size);
            cell->value = pzBlob;
          }
          break;

        case SQLITE_NULL:
          cell->value = NULL;
          break;

        default: {
          assert(0 && "unsupported type");
        }

        if (cell->type != SQLITE_NULL) {
          assert(cell->value);
        }
      }

      cell_prev = cell;
    }

    sto->cells = cell_head;
    assert(sto->column_names_);
  }
  else if (rc == SQLITE_DONE) {
    // nothing to do in this case
  }
  else {
    sto->error_ = true;
    sto->cells = NULL;
  }

  return 0;
}

Handle<Value> Statement::Step(const Arguments& args) {
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

int Statement::EIO_AfterFetchAll(eio_req *req) {
  HandleScope scope;
  ev_unref(EV_DEFAULT_UC);

  struct fetchall_request *fetchall_req
    = (struct fetchall_request *)(req->data);

  Statement *sto = fetchall_req->sto;

  struct row_node *cur = fetchall_req->rows;
  struct cell_node *cell = NULL;

  Local<Value> argv[2];

  if (fetchall_req->error != NULL) {
    argv[0] = Exception::Error(String::New(fetchall_req->error));
    argv[1] = Local<Value>::New(Undefined());
  }
  else {
    argv[0] = Local<Value>::New(Undefined());

    Persistent<Array> results = Persistent<Array>::New(Array::New());

    for (int row_count = 0; cur; row_count++, cur=cur->next) {
      Local<Object> row = Object::New();

      cell = cur->cells;

      // walk down the list
      for (int i = 0; cell; i++, cell=cell->next) {
        assert(cell);
        if (cell->type != SQLITE_NULL) {
          assert((void*)cell->value);
        }
        assert(sto->column_names_[i]);

        switch (cell->type) {
          case SQLITE_INTEGER:
            row->Set(String::NewSymbol((char*) sto->column_names_[i]),
                     Int32::New(*(int*) (cell->value)));
            break;

          case SQLITE_FLOAT:
            row->Set(String::NewSymbol(sto->column_names_[i]),
                     Number::New(*(double*) (cell->value)));
            break;

          case SQLITE_TEXT: {
              struct string_t *str = (struct string_t *) (cell->value);

              // str->bytes-1 to compensate for the NULL terminator
              row->Set(String::NewSymbol(sto->column_names_[i]),
                       String::New(str->data, str->bytes-1));
            }
            break;

          case SQLITE_NULL:
            row->Set(String::New(sto->column_names_[i]),
                Local<Value>::New(Null()));
            break;
        }
      }

      results->Set(Integer::New(row_count), row);
    }

    argv[1] = Local<Value>::New(results);
  }

//   unsigned int page_size_p;
//   unsigned long num_alloced_p;
//   unsigned long user_alloced_p;
//   unsigned long max_alloced_p;
//   unsigned long tot_alloced_p;
//   mpool_stats(fetchall_req->pool,
//           &page_size_p,
//           &num_alloced_p,
//           &user_alloced_p,
//           &max_alloced_p,
//           &tot_alloced_p);
//   printf(
//          "%u page size\n"
//          "%lu num allocated\n"
//          "%lu user allocated\n"
//          "%lu max allocated\n"
//          "%lu total allocated\n",
//          page_size_p,
//          num_alloced_p,
//          user_alloced_p,
//          max_alloced_p,
//          tot_alloced_p
//         );

  if (fetchall_req->pool) {
    int ret = mpool_close(fetchall_req->pool);
    if (ret != MPOOL_ERROR_NONE) {
      req->result = -1;
      argv[0] = Exception::Error(String::New(mpool_strerror(ret)));
      argv[1] = Local<Value>::New(Undefined());
    }
  }

  TryCatch try_catch;

  fetchall_req->cb->Call(Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  fetchall_req->cb.Dispose();

  sto->FreeColumnData();
  free(fetchall_req);

  return 0;
}

int Statement::EIO_FetchAll(eio_req *req) {
  struct fetchall_request *fetchall_req
    = (struct fetchall_request *)(req->data);

  Statement *sto = fetchall_req->sto;
  sqlite3_stmt *stmt = sto->stmt_;
  assert(stmt);
  int ret;


  int rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    /* open the pool */
    fetchall_req->pool = mpool_open(MPOOL_FLAG_USE_MAP_ANON
                                  , 0
                                  , NULL
                                  , &ret);
    if (fetchall_req->pool == NULL) {
      req->result = -1;
      fetchall_req->rows = NULL;
      fetchall_req->error = (char *) mpool_strerror(ret);
      return 0;
    }
  }
  else {
    fetchall_req->pool = NULL;
    fetchall_req->rows = NULL;
  }


  // We're going to be traversing a linked list in two dimensions.
  struct row_node *cur  = NULL
                , *prev = NULL
                , *head = NULL;

  for (;; rc = sqlite3_step(stmt)) {
    if (rc != SQLITE_ROW) break;

    if (!sto->column_names_) {
      sto->InitializeColumns();
    }

    cur = (struct row_node *) mpool_alloc
                                ( fetchall_req->pool
                                , sizeof(struct row_node)
                                , &ret
                                );
    cur->next = NULL;

    // If this is the first row, assign `cur` to `head` and `hold` head there
    // since it was the first result. Otherwise set the `next` field on
    // the`prev` pointer to attach the newly allocated element.
    (!head ? head : prev->next) = cur;

    struct cell_node *cell_head = NULL
                   , *cell_prev = NULL
                   , *cell      = NULL;

    for (int i = 0; i < sto->column_count_; i++) {
      cell = (struct cell_node *)
        mpool_alloc(fetchall_req->pool, sizeof(struct cell_node), &ret);

      // Same as above with the row linked list.
      (!cell_head ? cell_head : cell_prev->next) = cell;

      cell->type = sqlite3_column_type(sto->stmt_, i);
      cell->next = NULL;

      // TODO: Cache column data in the fetchall req struct.
      switch (cell->type) {
        case SQLITE_INTEGER:
          cell->value = (int *)
            mpool_alloc(fetchall_req->pool , sizeof(int) , &ret);

          * (int *) cell->value = sqlite3_column_int(stmt, i);
          break;

        case SQLITE_FLOAT:
          cell->value = (double *)
            mpool_alloc(fetchall_req->pool , sizeof(double) , &ret);

          * (double *) cell->value = sqlite3_column_double(stmt, i);
          break;

        case SQLITE_TEXT: {
            char *text = (char *) sqlite3_column_text(stmt, i);
            int size = 1+sqlite3_column_bytes(stmt, i);

            struct string_t *str = (struct string_t *)
              mpool_alloc(fetchall_req->pool, sizeof(size_t) + size, &ret);

            str->bytes = size;
            memcpy(str->data, text, size);
            cell->value = str;
          }
          break;

        case SQLITE_NULL:
          cell->value = NULL;
          break;

        default: {
          assert(0 && "unsupported type");
        }
      }

      assert(sto->column_names_[i]);

      cell_prev = cell;
    }

    cur->cells = cell_head;
    prev = cur;
  }

  // Test for errors
  if (rc != SQLITE_DONE) {
    const char *error = sqlite3_errmsg(sqlite3_db_handle(sto->stmt_));
    if (error == NULL) {
      error = "Unknown Error";
    }
    size_t errorSize = sizeof(char) * (strlen(error) + 1);
    fetchall_req->error = (char *)mpool_alloc(fetchall_req->pool, errorSize, &ret);
    if (fetchall_req->error != NULL) {
      memcpy(fetchall_req->error, error, errorSize);
    } else {
      fetchall_req->error = (char *) mpool_strerror(ret);
    }
    req->result = -1;
    return 0;
  }

  req->result = 0;
  fetchall_req->rows = head;

  return 0;
}

Handle<Value> Statement::FetchAll(const Arguments& args) {
  HandleScope scope;

  REQ_FUN_ARG(0, cb);

  struct fetchall_request *fetchall_req = (struct fetchall_request *)
      calloc(1, sizeof(struct fetchall_request));

  if (!fetchall_req) {
    V8::LowMemoryNotification();
    return ThrowException(Exception::Error(
          String::New("Could not allocate enough memory")));
  }

  fetchall_req->sto = ObjectWrap::Unwrap<Statement>(args.This());
  fetchall_req->cb = Persistent<Function>::New(cb);

  eio_custom(EIO_FetchAll, EIO_PRI_DEFAULT, EIO_AfterFetchAll, fetchall_req);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

// The following three methods must be called inside a HandleScope

bool Statement::HasCallback() {
  return ! handle_->GetHiddenValue(callback_sym).IsEmpty();
}

void Statement::SetCallback(Local<Function> cb) {
  handle_->SetHiddenValue(callback_sym, cb);
}

Local<Function> Statement::GetCallback() {
  Local<Value> cb_v = handle_->GetHiddenValue(callback_sym);
  assert(cb_v->IsFunction());
  Local<Function> cb = Local<Function>::Cast(cb_v);
  handle_->DeleteHiddenValue(callback_sym);
  return cb;
}
