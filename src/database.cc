// Copyright (c) 2010, Orlando Vazquez <ovazquez@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include <string.h>
#include <v8.h>
#include <node.h>
#include <node_events.h>

#include "macros.h"
#include "database.h"
#include "statement.h"
#include "deferred_call.h"

using namespace v8;
using namespace node;


Persistent<FunctionTemplate> Database::constructor_template;

void Database::Init(v8::Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(EventEmitter::constructor_template);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("Database"));

    NODE_SET_PROTOTYPE_METHOD(constructor_template, "open", Open);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "openSync", OpenSync);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "close", Close);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "closeSync", CloseSync);

    NODE_SET_PROTOTYPE_METHOD(constructor_template, "prepare", Prepare);

    target->Set(v8::String::NewSymbol("Database"),
            constructor_template->GetFunction());

    // insert/update execution result mask
    NODE_DEFINE_CONSTANT(target, EXEC_EMPTY);
    NODE_DEFINE_CONSTANT(target, EXEC_LAST_INSERT_ID);
    NODE_DEFINE_CONSTANT(target, EXEC_AFFECTED_ROWS);
}

Handle<Value> Database::New(const Arguments& args) {
    HandleScope scope;

    REQUIRE_ARGUMENT_STRING(0, filename);
    OPTIONAL_ARGUMENT_INTEGER(1, mode, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    Database* db = new Database();
    db->filename = std::string(*filename);
    db->open_mode = mode;

    args.This()->Set(String::NewSymbol("filename"), args[0]->ToString(), ReadOnly);
    args.This()->Set(String::NewSymbol("mode"), Integer::New(mode), ReadOnly);

    db->Wrap(args.This());

    return args.This();
}

inline void Database::RunQueue(std::queue<Call*> queue) {

}

void Database::ProcessQueue(Database* db) {
    while (!db->queue.empty()) {
        Call* call = db->queue.front();

        if (!(call->Data() & db->status)) {
            // The next task in the queue requires a different status than the
            // one we're currently in. Wait before invoking it.
            if (db->pending == 0) {
                EXCEPTION("Invalid function call sequence", SQLITE_MISUSE, exception);
                Local<Value> argv[] = { String::NewSymbol("error"), exception };
                Local<Function> fn = Local<Function>::Cast(db->handle_->Get(String::NewSymbol("emit")));

                TryCatch try_catch;
                fn->Call(db->handle_, 2, argv);
                if (try_catch.HasCaught()) {
                    FatalException(try_catch);
                }
                ev_unref(EV_DEFAULT_UC);
                db->Unref();
                db->queue.pop();
                delete call;
            }
            break;
        }

        if (call->Mode() == Deferred::Exclusive && db->pending > 0) {
            // We have to wait for the pending tasks to complete before we
            // execute the exclusive task.
            // break;
            break;
        }

        ev_unref(EV_DEFAULT_UC);
        db->Unref();

        TryCatch try_catch;
        call->Invoke();
        if (try_catch.HasCaught()) {
            FatalException(try_catch);
        }
        db->queue.pop();
        delete call;
    }
}

Handle<Value> Database::Open(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    REQUIRE_ARGUMENT_FUNCTION(0, callback);

    // Make sure that node doesn't exit before all elements in the queue have
    // been dealt with.
    db->Ref();
    ev_ref(EV_DEFAULT_UC);

    if (db->status != IsClosed) {
        db->queue.push(new Call(args, IsClosed, Deferred::Exclusive));
    }
    else {
        db->status = IsOpening;
        db->pending++;
        Baton* baton = new Baton(db, Persistent<Function>::New(callback));
        eio_custom(EIO_Open, EIO_PRI_DEFAULT, EIO_AfterOpen, baton);
    }

    return args.This();
}

bool Database::Open(Database* db) {
    db->error_status = sqlite3_open_v2(
        db->filename.c_str(),
        &db->handle,
        SQLITE_OPEN_FULLMUTEX | db->open_mode,
        NULL
    );

    if (db->error_status != SQLITE_OK) {
        db->error_message = std::string(sqlite3_errmsg(db->handle));
        return false;
    }
    else {
        return true;
    }
}

Handle<Value> Database::OpenSync(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    REQUIRE_ARGUMENT_FUNCTION(0, callback);

    if (db->status == IsClosed) {
        Local<Value> argv[1];
        if (Open(db)) {
            db->status = IsOpen;
            argv[0] = Local<Value>::New(Null());
        }
        else {
            EXCEPTION(db->error_message.c_str(), db->error_status, exception);
            argv[0] = exception;
        }

        TRY_CATCH_CALL(args.This(), callback, 1, argv);
        ProcessQueue(db);
    }
    else {
        return ThrowException(Exception::Error(
            String::New("Database is already open")));
    }

    return args.This();
}

int Database::EIO_Open(eio_req *req) {
    Baton* baton = static_cast<Baton*>(req->data);
    Open(baton->db);
    return 0;
}

int Database::EIO_AfterOpen(eio_req *req) {
    HandleScope scope;
    Baton* baton = static_cast<Baton*>(req->data);
    Database* db = baton->db;

    ev_unref(EV_DEFAULT_UC);
    db->Unref();
    db->pending--;

    Local<Value> argv[1];
    if (db->error_status == SQLITE_OK) {
        db->status = IsOpen;
        argv[0] = Local<Value>::New(Null());
    }
    else {
        db->status = IsClosed;
        EXCEPTION(db->error_message.c_str(), db->error_status, exception);
        argv[0] = exception;
    }

    TRY_CATCH_CALL(db->handle_, baton->callback, 1, argv);
    ProcessQueue(db);

    baton->callback.Dispose();
    delete baton;

    return 0;
}



Handle<Value> Database::Close(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    REQUIRE_ARGUMENT_FUNCTION(0, callback);

    // Make sure that node doesn't exit before all elements in the queue have
    // been dealt with.
    db->Ref();
    ev_ref(EV_DEFAULT_UC);

    if (db->status != IsOpen || db->pending > 0) {
        db->queue.push(new Call(args, IsOpen, Deferred::Exclusive));
    }
    else {
        db->status = IsClosing;
        db->pending++;
        Baton* baton = new Baton(db, Persistent<Function>::New(callback));
        eio_custom(EIO_Close, EIO_PRI_DEFAULT, EIO_AfterClose, baton);
    }

    return args.This();
}

bool Database::Close(Database* db) {
    assert(db->handle);
    db->error_status = sqlite3_close(db->handle);

    if (db->error_status != SQLITE_OK) {
        db->error_message = std::string(sqlite3_errmsg(db->handle));
        return false;
    }
    else {
        db->handle = NULL;
        return true;
    }
}


Handle<Value> Database::CloseSync(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    REQUIRE_ARGUMENT_FUNCTION(0, callback);

    if (db->status == IsOpen && db->pending == 0) {
        Local<Value> argv[1];
        if (Close(db)) {
            db->status = IsClosed;
            argv[0] = Local<Value>::New(Null());
        }
        else {
            EXCEPTION(db->error_message.c_str(), db->error_status, exception);
            argv[0] = exception;
        }

        TRY_CATCH_CALL(args.This(), callback, 1, argv);
        ProcessQueue(db);
    }
    else {
        db->queue.push(new Call(args, IsOpen, Deferred::Exclusive));
    }
    return args.This();
}

int Database::EIO_Close(eio_req *req) {
    Baton* baton = static_cast<Baton*>(req->data);
    Close(baton->db);
    return 0;
}

int Database::EIO_AfterClose(eio_req *req) {
    HandleScope scope;
    Baton* baton = static_cast<Baton*>(req->data);
    Database* db = baton->db;

    ev_unref(EV_DEFAULT_UC);
    db->Unref();
    db->pending--;

    Local<Value> argv[1];
    if (db->error_status != SQLITE_OK) {
        db->status = IsOpen;
        EXCEPTION(db->error_message.c_str(), db->error_status, exception);
        argv[0] = exception;
    }
    else {
        db->status = IsClosed;
        argv[0] = Local<Value>::New(Null());
    }

    TRY_CATCH_CALL(db->handle_, baton->callback, 1, argv);
    ProcessQueue(db);
    baton->callback.Dispose();
    delete baton;

    return 0;
}

// // TODO: libeio'fy

// Hooks
//   static int CommitHook(void* v_this) {
//     HandleScope scope;
//     Database* db = static_cast<Database*>(v_this);
//     db->Emit(String::New("commit"), 0, NULL);
//     // TODO: allow change in return value to convert to rollback...somehow
//     return 0;
//   }
//
//   static void RollbackHook(void* v_this) {
//     HandleScope scope;
//     Database* db = static_cast<Database*>(v_this);
//     db->Emit(String::New("rollback"), 0, NULL);
//   }
//
//   static void UpdateHook(void* v_this, int operation, const char* database,
//                          const char* table, sqlite_int64 rowid) {
//     HandleScope scope;
//     Database* db = static_cast<Database*>(v_this);
//     Local<Value> args[] = { Int32::New(operation), String::New(database),
//                             String::New(table), Number::New(rowid) };
//     db->Emit(String::New("update"), 4, args);
//   }

int Database::EIO_AfterPrepareAndStep(eio_req *req) {
    ev_unref(EV_DEFAULT_UC);
    struct prepare_request *prep_req = (struct prepare_request *)(req->data);
    HandleScope scope;

    Local<Value> argv[2];
    int argc = 0;

    // if the prepare failed
    if (req->result != SQLITE_OK) {
        argv[0] = Exception::Error(
            String::New(sqlite3_errmsg(prep_req->db->handle)));
        argc = 1;

    }
    else {
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

        }
        else {
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

    prep_req->db->Unref();
    prep_req->cb->Call(Context::GetCurrent()->Global(), argc, argv);

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    prep_req->cb.Dispose();
    free(prep_req);

    return 0;
}

int Database::EIO_PrepareAndStep(eio_req *req) {
    struct prepare_request *prep_req = (struct prepare_request *)(req->data);

    prep_req->stmt = NULL;
    prep_req->tail = NULL;
    sqlite3* db = prep_req->db->handle;

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

Handle<Value> Database::PrepareAndStep(const Arguments& args) {
    HandleScope scope;

    REQUIRE_ARGUMENT_STRING(0, sql);
    REQUIRE_ARGUMENT_FUNCTION(1, cb);
    OPTIONAL_ARGUMENT_INTEGER(2, mode, EXEC_EMPTY);

    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    struct prepare_request *prep_req = (struct prepare_request *)
        calloc(1, sizeof(struct prepare_request) + sql.length());

    if (!prep_req) {
        V8::LowMemoryNotification();
        return ThrowException(Exception::Error(
            String::New("Could not allocate enough memory")));
    }

    strcpy(prep_req->sql, *sql);
    prep_req->cb = Persistent<Function>::New(cb);
    prep_req->db = db;
    prep_req->mode = mode;

    eio_custom(EIO_PrepareAndStep, EIO_PRI_DEFAULT, EIO_AfterPrepareAndStep, prep_req);

    ev_ref(EV_DEFAULT_UC);
    db->Ref();

    return Undefined();
}

int Database::EIO_AfterPrepare(eio_req *req) {
    ev_unref(EV_DEFAULT_UC);
    struct prepare_request *prep_req = (struct prepare_request *)(req->data);
    HandleScope scope;

    Local<Value> argv[3];
    int argc = 0;

    // if the prepare failed
    if (req->result != SQLITE_OK) {
        argv[0] = Exception::Error(
                  String::New(sqlite3_errmsg(prep_req->db->handle)));
        argc = 1;
    }
    else {
        argv[0] = External::New(prep_req->stmt);
        argv[1] = Integer::New(-1);
        argv[2] = Integer::New(prep_req->mode);
        Persistent<Object> statement(
            Statement::constructor_template->GetFunction()->NewInstance(3, argv));

        if (prep_req->tail) {
            statement->Set(String::New("tail"), String::New(prep_req->tail));
        }

        argc = 2;
        argv[0] = Local<Value>::New(Undefined());
        argv[1] = Local<Value>::New(statement);
    }

    TryCatch try_catch;

    prep_req->db->Unref();
    prep_req->cb->Call(Context::GetCurrent()->Global(), argc, argv);

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    prep_req->cb.Dispose();
    free(prep_req);

    return 0;
}
int Database::EIO_Prepare(eio_req *req) {
    struct prepare_request *prep_req = (struct prepare_request *)(req->data);

    prep_req->stmt = NULL;
    prep_req->tail = NULL;
    sqlite3* db = prep_req->db->handle;

    int rc = sqlite3_prepare_v2(db, prep_req->sql, -1,
                &(prep_req->stmt), &(prep_req->tail));

    req->result = rc;

    prep_req->lastInsertId = 0;
    prep_req->affectedRows = 0;

    // load custom properties
    if (prep_req->mode & EXEC_LAST_INSERT_ID)
        prep_req->lastInsertId = sqlite3_last_insert_rowid(db);
    if (prep_req->mode & EXEC_AFFECTED_ROWS)
        prep_req->affectedRows = sqlite3_changes(db);

    return 0;
}

// Statement#prepare(sql, [ options ,] callback);
Handle<Value> Database::Prepare(const Arguments& args) {
    HandleScope scope;
    Local<Object> options;
    Local<Function> cb;
    int mode;

    REQUIRE_ARGUMENT_STRING(0, sql);

    // middle argument could be options or
    switch (args.Length()) {
        case 2:
            if (!args[1]->IsFunction()) {
                return ThrowException(Exception::TypeError(
                        String::New("Argument 1 must be a function")));
            }
            cb = Local<Function>::Cast(args[1]);
            options = Object::New();
            break;

        case 3:
            if (!args[1]->IsObject()) {
                return ThrowException(Exception::TypeError(
                        String::New("Argument 1 must be an object")));
            }
            options = Local<Function>::Cast(args[1]);

            if (!args[2]->IsFunction()) {
                return ThrowException(Exception::TypeError(
                        String::New("Argument 2 must be a function")));
            }
            cb = Local<Function>::Cast(args[2]);
            break;
    }

    mode = EXEC_EMPTY;

    if (options->Get(String::New("lastInsertRowID"))->IsTrue()) {
        mode |= EXEC_LAST_INSERT_ID;
    }
    if (options->Get(String::New("affectedRows"))->IsTrue())  {
        mode |= EXEC_AFFECTED_ROWS;
    }

    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    struct prepare_request *prep_req = (struct prepare_request *)
        calloc(1, sizeof(struct prepare_request) + sql.length());

    if (!prep_req) {
        V8::LowMemoryNotification();
        return ThrowException(Exception::Error(
            String::New("Could not allocate enough memory")));
    }

    strcpy(prep_req->sql, *sql);
    prep_req->cb = Persistent<Function>::New(cb);
    prep_req->db = db;
    prep_req->mode = mode;

    eio_custom(EIO_Prepare, EIO_PRI_DEFAULT, EIO_AfterPrepare, prep_req);

    ev_ref(EV_DEFAULT_UC);
    db->Ref();

    return Undefined();
}

/**
 * Override this so that we can properly close the database when this object
 * gets garbage collected.
 */
void Database::Wrap(Handle<Object> handle) {
    assert(handle_.IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    handle_ = Persistent<Object>::New(handle);
    handle_->SetPointerInInternalField(0, this);
    handle_.MakeWeak(this, Destruct);
}

void Database::Destruct(Persistent<Value> value, void *data) {
    Database* db = static_cast<Database*>(data);
    if (db->handle) {
        eio_custom(EIO_Close, EIO_PRI_DEFAULT, EIO_AfterDestruct, db);
        ev_ref(EV_DEFAULT_UC);
    }
    else {
        delete db;
    }
}

int Database::EIO_AfterDestruct(eio_req *req) {
    Database* db = static_cast<Database*>(req->data);
    ev_unref(EV_DEFAULT_UC);
    delete db;
    return 0;
}
