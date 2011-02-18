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

    target->Set(v8::String::NewSymbol("Statement"),
                constructor_template->GetFunction());
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
    PrepareBaton* baton = new PrepareBaton(db, Local<Function>::Cast(args[3]));
    baton->stmt = stmt;
    baton->sql = std::string(*String::Utf8Value(sql));
    Database::Schedule(db, EIO_BeginPrepare, baton, false);

    return args.This();
}


void Statement::EIO_BeginPrepare(Database::Baton* baton) {
    assert(baton->db->open);
    assert(!baton->db->locked);
    static_cast<PrepareBaton*>(baton)->stmt->Ref();
    ev_ref(EV_DEFAULT_UC);
    fprintf(stderr, "Prepare started\n");
    eio_custom(EIO_Prepare, EIO_PRI_DEFAULT, EIO_AfterPrepare, baton);
}

int Statement::EIO_Prepare(eio_req *req) {
    PrepareBaton* baton = static_cast<PrepareBaton*>(req->data);
    Database* db = baton->db;
    Statement* stmt = baton->stmt;

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

    stmt->Unref();
    ev_unref(EV_DEFAULT_UC);

    // Local<Value> argv[1];
    // if (baton->status != SQLITE_OK) {
    //     EXCEPTION(String::New(baton->message), baton->status, exception);
    //     argv[0] = exception;
    // }
    // else {
    //     db->open = false;
    //     // Leave db->locked to indicate that this db object has reached
    //     // the end of its life.
    //     argv[0] = Local<Value>::New(Null());
    // }
    //
    // // Fire callbacks.
    // if (!baton->callback.IsEmpty()) {
    //     TRY_CATCH_CALL(db->handle_, baton->callback, 1, argv);
    // }
    // else if (db->open) {
    //     Local<Value> args[] = { String::NewSymbol("error"), argv[0] };
    //     EMIT_EVENT(db->handle_, 2, args);
    // }
    //
    // if (!db->open) {
    //     Local<Value> args[] = { String::NewSymbol("close"), argv[0] };
    //     EMIT_EVENT(db->handle_, 1, args);
    //     Process(db);
    // }

    fprintf(stderr, "Prepare completed\n");

    // V8::AdjustAmountOfExternalAllocatedMemory(10000000);

    Database::Process(db);

    delete baton;

    return 0;
}

/**
 * Override this so that we can properly finalize the statement when it
 * gets garbage collected.
 */
void Statement::Wrap(Handle<Object> handle) {
    assert(handle_.IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    handle_ = Persistent<Object>::New(handle);
    handle_->SetPointerInInternalField(0, this);
    handle_.MakeWeak(this, Destruct);
}

inline void Statement::MakeWeak (void) {
    handle_.MakeWeak(this, Destruct);
}

void Statement::Unref() {
    assert(!handle_.IsEmpty());
    assert(!handle_.IsWeak());
    assert(refs_ > 0);
    if (--refs_ == 0) { MakeWeak(); }
}

void Statement::Destruct(Persistent<Value> value, void *data) {
    Statement* stmt = static_cast<Statement*>(data);
    if (stmt->handle) {
        eio_custom(EIO_Destruct, EIO_PRI_DEFAULT, EIO_AfterDestruct, stmt);
        ev_ref(EV_DEFAULT_UC);
    }
    else {
        delete stmt;
    }
}

int Statement::EIO_Destruct(eio_req *req) {
    Statement* stmt = static_cast<Statement*>(req->data);

    fprintf(stderr, "Auto-Finalizing handle\n");
    sqlite3_finalize(stmt->handle);
    stmt->handle = NULL;

    return 0;
}

int Statement::EIO_AfterDestruct(eio_req *req) {
    Statement* stmt = static_cast<Statement*>(req->data);
    ev_unref(EV_DEFAULT_UC);
    delete stmt;
    return 0;
}
