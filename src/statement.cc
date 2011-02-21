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

Persistent<FunctionTemplate> Statement::constructor_template;

void Statement::Init(v8::Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(EventEmitter::constructor_template);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("Statement"));

    NODE_SET_PROTOTYPE_METHOD(constructor_template, "run", Run);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "finalize", Finalize);

    target->Set(v8::String::NewSymbol("Statement"),
                constructor_template->GetFunction());
}

void Statement::Process() {
    if (finalized && !queue.empty()) {
        return CleanQueue();
    }

    while (prepared && !locked && !queue.empty()) {
        Call* call = queue.front();
        queue.pop();

        call->callback(call->baton);
        delete call;
    }
}

void Statement::Schedule(EIO_Callback callback, Baton* baton) {
    if (finalized) {
        queue.push(new Call(callback, baton));
        CleanQueue();
    }
    else if (!prepared || locked) {
        queue.push(new Call(callback, baton));
    }
    else {
        callback(baton);
    }
}

template <class T> void Statement::Error(T* baton) {
    EXCEPTION(String::New(baton->message.c_str()), baton->status, exception);

    if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
        Local<Value> argv[] = { exception };
        TRY_CATCH_CALL(baton->stmt->handle_, baton->callback, 1, argv);
    }
    else {
        Local<Value> argv[] = { String::NewSymbol("error"), exception };
        EMIT_EVENT(baton->stmt->handle_, 2, argv);
    }
}

// { Database db, String sql, Array params, Function callback }
Handle<Value> Statement::New(const Arguments& args) {
    HandleScope scope;

    // if (args.Length() < 1 || !args[0]->IsString()) {
    //     return ThrowException(Exception::TypeError(
    //         String::New("First argument must be a SQL query"))
    //     );
    // }
    //
    // Local<Function> callback;
    // int last = args.Length() - 1;
    // if (args.Length() >= 2 && args[last]->IsFunction()) {
    //     callback = Local<Function>::Cast(args[last--]);
    // }
    //
    // // Process any optional arguments
    // for (int i = 1; i <= last; i++) {
    //
    // }

    int length = args.Length();

    if (length <= 0 || !Database::HasInstance(args[0])) {
        return ThrowException(Exception::TypeError(
            String::New("First argument must be a Database object")));
    }
    else if (length <= 1 || !args[1]->IsString()) {
        return ThrowException(Exception::TypeError(
            String::New("Second argument must be a SQL query")));
    }
    else if (length <= 2 || !args[2]->IsObject()) {
        return ThrowException(Exception::TypeError(
            String::New("Third argument must be an array or object of parameters")));
    }
    else if (length > 3 && !args[3]->IsUndefined() && !args[3]->IsFunction()) {
        return ThrowException(Exception::TypeError(
            String::New("Fourth argument must be a function")));
    }

    Database* db = ObjectWrap::Unwrap<Database>(args[0]->ToObject());
    Local<String> sql = Local<String>::Cast(args[1]);

    args.This()->Set(String::NewSymbol("sql"), Persistent<String>::New(sql), ReadOnly);

    Statement* stmt = new Statement(db);
    stmt->Wrap(args.This());
    PrepareBaton* baton = new PrepareBaton(db, Local<Function>::Cast(args[3]), stmt);
    baton->sql = std::string(*String::Utf8Value(sql));
    db->Schedule(EIO_BeginPrepare, baton, false);

    return args.This();
}


void Statement::EIO_BeginPrepare(Database::Baton* baton) {
    assert(baton->db->open);
    assert(!baton->db->locked);
    fprintf(stderr, "Prepare started\n");
    eio_custom(EIO_Prepare, EIO_PRI_DEFAULT, EIO_AfterPrepare, baton);
}

int Statement::EIO_Prepare(eio_req *req) {
    PrepareBaton* baton = static_cast<PrepareBaton*>(req->data);
    Database* db = baton->db;
    Statement* stmt = baton->stmt;

    // In case preparing fails, we use a mutex to make sure we get the associated
    // error message.
    sqlite3_mutex* mtx = sqlite3_db_mutex(db->handle);
    sqlite3_mutex_enter(mtx);

    baton->status = sqlite3_prepare_v2(
        db->handle,
        baton->sql.c_str(),
        baton->sql.size(),
        &stmt->handle,
        NULL
    );

    if (baton->status != SQLITE_OK) {
        baton->message = std::string(sqlite3_errmsg(db->handle));
        stmt->handle = NULL;
    }

    sqlite3_mutex_leave(mtx);

    return 0;
}

int Statement::EIO_AfterPrepare(eio_req *req) {
    HandleScope scope;
    PrepareBaton* baton = static_cast<PrepareBaton*>(req->data);
    Database* db = baton->db;
    Statement* stmt = baton->stmt;


    if (baton->status != SQLITE_OK) {
        Error(baton);
        stmt->Finalize();
    }
    else {
        stmt->prepared = true;
        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            Local<Value> argv[] = { Local<Value>::New(Null()) };
            TRY_CATCH_CALL(stmt->handle_, baton->callback, 1, argv);
        }
        stmt->Process();
    }

    db->Process();

    delete baton;
    return 0;
}

Handle<Value> Statement::Run(const Arguments& args) {
    HandleScope scope;
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    RowBaton* baton = new RowBaton(stmt, callback);
    stmt->Schedule(EIO_BeginRun, baton);

    return args.This();
}

void Statement::EIO_BeginRun(Baton* baton) {
    assert(!baton->stmt->locked);
    assert(!baton->stmt->finalized);
    assert(baton->stmt->prepared);
    baton->stmt->locked = true;
    eio_custom(EIO_Run, EIO_PRI_DEFAULT, EIO_AfterRun, baton);
}

void Statement::GetRow(Result::Row* row, sqlite3_stmt* stmt) {
    int rows = sqlite3_column_count(stmt);

    for (int i = 0; i < rows; i++) {
        int type = sqlite3_column_type(stmt, i);
        switch (type) {
            case SQLITE_INTEGER: {
                row->push_back(new Result::Integer(sqlite3_column_int(stmt, i)));
            }   break;
            case SQLITE_FLOAT: {
                row->push_back(new Result::Float(sqlite3_column_double(stmt, i)));
            }   break;
            case SQLITE_TEXT: {
                const char* text = (const char*)sqlite3_column_text(stmt, i);
                int length = sqlite3_column_bytes(stmt, i);
                row->push_back(new Result::Text(length, text));
            } break;
            case SQLITE_BLOB: {
                const void* blob = sqlite3_column_blob(stmt, i);
                int length = sqlite3_column_bytes(stmt, i);
                row->push_back(new Result::Blob(length, blob));
            }   break;
            case SQLITE_NULL: {
                row->push_back(new Result::Null());
            }   break;
            default:
                assert(false);
        }
    }
}

int Statement::EIO_Run(eio_req *req) {
    RowBaton* baton = static_cast<RowBaton*>(req->data);
    Statement* stmt = baton->stmt;
    Database* db = stmt->db;

    // In case preparing fails, we use a mutex to make sure we get the associated
    // error message.
    sqlite3_mutex* mtx = sqlite3_db_mutex(db->handle);
    sqlite3_mutex_enter(mtx);

    baton->status = sqlite3_step(stmt->handle);

    if (!(baton->status == SQLITE_ROW || baton->status == SQLITE_DONE)) {
        baton->message = std::string(sqlite3_errmsg(db->handle));
    }

    sqlite3_mutex_leave(mtx);

    if (baton->status == SQLITE_ROW) {
        // Acquire one result row before returning.
        GetRow(&baton->row, stmt->handle);
    }

    return 0;
}

Local<Array> Statement::RowToJS(Result::Row* row) {
    Local<Array> result(Array::New(row->size()));

    Result::Row::iterator it = row->begin();
    for (int i = 0; it < row->end(); it++, i++) {
        Result::Field* field = *it;
        switch (field->type) {
            case SQLITE_INTEGER: {
                result->Set(i, Local<Integer>(Integer::New(((Result::Integer*)field)->value)));
            } break;
            case SQLITE_FLOAT: {
                result->Set(i, Local<Number>(Number::New(((Result::Float*)field)->value)));
            } break;
            case SQLITE_TEXT: {
                result->Set(i, Local<String>(String::New(((Result::Text*)field)->value.c_str(), ((Result::Text*)field)->value.size())));
            } break;
            // case SQLITE_BLOB: {
            //     result->Set(i, Local<String>(String::New(((Result::Text*)field)->value.c_str())));
            // } break;
            case SQLITE_NULL: {
                result->Set(i, Local<Value>::New(Null()));
            } break;
        }
    }

    return result;
}

int Statement::EIO_AfterRun(eio_req *req) {
    HandleScope scope;
    RowBaton* baton = static_cast<RowBaton*>(req->data);
    Statement* stmt = baton->stmt;

    if (baton->status != SQLITE_ROW && baton->status != SQLITE_DONE) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            if (baton->status == SQLITE_ROW) {
                // Create the result array from the data we acquired.
                Local<Value> argv[] = { Local<Value>::New(Null()), RowToJS(&baton->row) };
                TRY_CATCH_CALL(stmt->handle_, baton->callback, 2, argv);
            }
            else {
                Local<Value> argv[] = { Local<Value>::New(Null()) };
                TRY_CATCH_CALL(stmt->handle_, baton->callback, 1, argv);
            }
        }
    }

    stmt->locked = false;
    stmt->Process();

    delete baton;
    return 0;
}

Handle<Value> Statement::Finalize(const Arguments& args) {
    HandleScope scope;
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());
    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(stmt, callback);
    stmt->Schedule(Finalize, baton);

    return stmt->db->handle_;
}

void Statement::Finalize(Baton* baton) {
    baton->stmt->Finalize();

    // Fire callback in case there was one.
    if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
        TRY_CATCH_CALL(baton->stmt->handle_, baton->callback, 0, NULL);
    }

    delete baton;
}

void Statement::Finalize() {
    assert(!finalized);
    finalized = true;
    fprintf(stderr, "Statement destruct\n");
    CleanQueue();
    // Finalize returns the status code of the last operation. We already fired
    // error events in case those failed.
    sqlite3_finalize(handle);
    handle = NULL;
    db->pending--;
    db->Process();
    db->Unref();
}

void Statement::CleanQueue() {
    if (prepared && !queue.empty()) {
        // This statement has already been prepared and is now finalized.
        // Fire error for all remaining items in the queue.
        EXCEPTION(String::New("Statement is already finalized"), SQLITE_MISUSE, exception);
        Local<Value> argv[] = { exception };
        bool called = false;

        // Clear out the queue so that this object can get GC'ed.
        while (!queue.empty()) {
            Call* call = queue.front();
            queue.pop();

            if (prepared && !call->baton->callback.IsEmpty() &&
                call->baton->callback->IsFunction()) {
                TRY_CATCH_CALL(handle_, call->baton->callback, 1, argv);
                called = true;
            }

            // We don't call the actual callback, so we have to make sure that
            // the baton gets destroyed.
            delete call->baton;
            delete call;
        }

        // When we couldn't call a callback function, emit an error on the
        // Statement object.
        if (!called) {
            Local<Value> args[] = { String::NewSymbol("error"), exception };
            EMIT_EVENT(handle_, 2, args);
        }
    }
    else while (!queue.empty()) {
        // Just delete all items in the queue; we already fired an event when
        // preparing the statement failed.
        Call* call = queue.front();
        queue.pop();

        // We don't call the actual callback, so we have to make sure that
        // the baton gets destroyed.
        delete call->baton;
        delete call;
    }
}

