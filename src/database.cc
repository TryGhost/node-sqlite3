#include <string.h>
#include <v8.h>
#include <node.h>
#include <node_events.h>

#include "macros.h"
#include "database.h"
#include "statement.h"

using namespace node_sqlite3;

Persistent<FunctionTemplate> Database::constructor_template;

void Database::Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(EventEmitter::constructor_template);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("Database"));

    NODE_SET_PROTOTYPE_METHOD(constructor_template, "close", Close);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "exec", Exec);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "loadExtension", LoadExtension);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "serialize", Serialize);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "parallelize", Parallelize);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "configure", Configure);

    target->Set(String::NewSymbol("Database"),
        constructor_template->GetFunction());
}

void Database::Process() {
    if (!open && locked && !queue.empty()) {
        EXCEPTION(String::New("Database handle is closed"), SQLITE_MISUSE, exception);
        Local<Value> argv[] = { exception };
        bool called = false;

        // Call all callbacks with the error object.
        while (!queue.empty()) {
            Call* call = queue.front();
            if (!call->baton->callback.IsEmpty() && call->baton->callback->IsFunction()) {
                TRY_CATCH_CALL(handle_, call->baton->callback, 1, argv);
                called = true;
            }
            queue.pop();
            // We don't call the actual callback, so we have to make sure that
            // the baton gets destroyed.
            delete call->baton;
            delete call;
        }

        // When we couldn't call a callback function, emit an error on the
        // Database object.
        if (!called) {
            Local<Value> args[] = { String::NewSymbol("error"), exception };
            EMIT_EVENT(handle_, 2, args);
        }
        return;
    }

    while (open && (!locked || pending == 0) && !queue.empty()) {
        Call* call = queue.front();

        if (call->exclusive && pending > 0) {
            break;
        }

        queue.pop();
        locked = call->exclusive;
        call->callback(call->baton);
        delete call;

        if (locked) break;
    }
}

void Database::Schedule(EIO_Callback callback, Baton* baton, bool exclusive) {
    if (!open && locked) {
        EXCEPTION(String::New("Database is closed"), SQLITE_MISUSE, exception);
        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            Local<Value> argv[] = { exception };
            TRY_CATCH_CALL(handle_, baton->callback, 1, argv);
        }
        else {
            Local<Value> argv[] = { String::NewSymbol("error"), exception };
            EMIT_EVENT(handle_, 2, argv);
        }
        return;
    }

    if (!open || ((locked || exclusive || serialize) && pending > 0)) {
        queue.push(new Call(callback, baton, exclusive || serialize));
    }
    else {
        locked = exclusive;
        callback(baton);
    }
}

Handle<Value> Database::New(const Arguments& args) {
    HandleScope scope;

    if (!Database::HasInstance(args.This())) {
        return ThrowException(Exception::TypeError(
            String::New("Use the new operator to create new Database objects"))
        );
    }

    REQUIRE_ARGUMENT_STRING(0, filename);
    int pos = 1;

    int mode = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    if (args.Length() >= pos && args[pos]->IsInt32()) {
        mode = args[pos++]->Int32Value();
    }

    Local<Function> callback;
    if (args.Length() >= pos && args[pos]->IsFunction()) {
        callback = Local<Function>::Cast(args[pos++]);
    }

    Database* db = new Database();
    db->Wrap(args.This());

    args.This()->Set(String::NewSymbol("filename"), args[0]->ToString(), ReadOnly);
    args.This()->Set(String::NewSymbol("mode"), Integer::New(mode), ReadOnly);

    // Start opening the database.
    OpenBaton* baton = new OpenBaton(db, callback, *filename, SQLITE_OPEN_FULLMUTEX | mode);
    EIO_BeginOpen(baton);

    return args.This();
}

void Database::EIO_BeginOpen(Baton* baton) {
    eio_custom(EIO_Open, EIO_PRI_DEFAULT, EIO_AfterOpen, baton);
}

int Database::EIO_Open(eio_req *req) {
    OpenBaton* baton = static_cast<OpenBaton*>(req->data);
    Database* db = baton->db;

    baton->status = sqlite3_open_v2(
        baton->filename.c_str(),
        &db->handle,
        baton->mode,
        NULL
    );

    if (baton->status != SQLITE_OK) {
        baton->message = std::string(sqlite3_errmsg(db->handle));
        sqlite3_close(db->handle);
        db->handle = NULL;
    }

    return 0;
}

int Database::EIO_AfterOpen(eio_req *req) {
    HandleScope scope;
    OpenBaton* baton = static_cast<OpenBaton*>(req->data);
    Database* db = baton->db;

    Local<Value> argv[1];
    if (baton->status != SQLITE_OK) {
        EXCEPTION(String::New(baton->message.c_str()), baton->status, exception);
        argv[0] = exception;
    }
    else {
        db->open = true;
        argv[0] = Local<Value>::New(Null());
    }

    if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
        TRY_CATCH_CALL(db->handle_, baton->callback, 1, argv);
    }
    else if (!db->open) {
        Local<Value> args[] = { String::NewSymbol("error"), argv[0] };
        EMIT_EVENT(db->handle_, 2, args);
    }

    if (db->open) {
        Local<Value> args[] = { String::NewSymbol("open") };
        EMIT_EVENT(db->handle_, 1, args);
        db->Process();
    }

    delete baton;
    return 0;
}

Handle<Value> Database::Close(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());
    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(db, callback);
    db->Schedule(EIO_BeginClose, baton, true);

    return args.This();
}

void Database::EIO_BeginClose(Baton* baton) {
    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->handle);
    assert(baton->db->pending == 0);

    if (baton->db->debug_trace) {
        delete baton->db->debug_trace;
        baton->db->debug_trace = NULL;
    }

    eio_custom(EIO_Close, EIO_PRI_DEFAULT, EIO_AfterClose, baton);
}

int Database::EIO_Close(eio_req *req) {
    Baton* baton = static_cast<Baton*>(req->data);
    Database* db = baton->db;

    baton->status = sqlite3_close(db->handle);

    if (baton->status != SQLITE_OK) {
        baton->message = std::string(sqlite3_errmsg(db->handle));
    }
    else {
        db->handle = NULL;
    }
    return 0;
}

int Database::EIO_AfterClose(eio_req *req) {
    HandleScope scope;
    Baton* baton = static_cast<Baton*>(req->data);
    Database* db = baton->db;

    Local<Value> argv[1];
    if (baton->status != SQLITE_OK) {
        EXCEPTION(String::New(baton->message.c_str()), baton->status, exception);
        argv[0] = exception;
    }
    else {
        db->open = false;
        // Leave db->locked to indicate that this db object has reached
        // the end of its life.
        argv[0] = Local<Value>::New(Null());
    }

    // Fire callbacks.
    if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
        TRY_CATCH_CALL(db->handle_, baton->callback, 1, argv);
    }
    else if (db->open) {
        Local<Value> args[] = { String::NewSymbol("error"), argv[0] };
        EMIT_EVENT(db->handle_, 2, args);
    }

    if (!db->open) {
        Local<Value> args[] = { String::NewSymbol("close"), argv[0] };
        EMIT_EVENT(db->handle_, 1, args);
        db->Process();
    }

    delete baton;
    return 0;
}

Handle<Value> Database::Serialize(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());
    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    bool before = db->serialize;
    db->serialize = true;

    if (!callback.IsEmpty() && callback->IsFunction()) {
        TRY_CATCH_CALL(args.This(), callback, 0, NULL);
        db->serialize = before;
    }

    db->Process();

    return args.This();
}

Handle<Value> Database::Parallelize(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());
    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    bool before = db->serialize;
    db->serialize = false;

    if (!callback.IsEmpty() && callback->IsFunction()) {
        TRY_CATCH_CALL(args.This(), callback, 0, NULL);
        db->serialize = before;
    }

    db->Process();

    return args.This();
}

Handle<Value> Database::Configure(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    REQUIRE_ARGUMENTS(2);

    if (args[0]->Equals(String::NewSymbol("trace"))) {
        Local<Function> handle;
        Baton* baton = new Baton(db, handle);
        db->Schedule(RegisterTraceCallback, baton);
    }
    else {
        return ThrowException(Exception::Error(String::Concat(
            args[0]->ToString(),
            String::NewSymbol(" is not a valid configuration option")
        )));
    }

    db->Process();

    return args.This();
}

void Database::RegisterTraceCallback(Baton* baton) {
    assert(baton->db->open);
    assert(baton->db->handle);
    Database* db = baton->db;

    if (db->debug_trace == NULL) {
        // Add it.
        db->debug_trace = new AsyncTrace(db, TraceCallback);
        sqlite3_trace(db->handle, TraceCallback, db);
    }
    else {
        // Remove it.
        sqlite3_trace(db->handle, NULL, NULL);
        delete db->debug_trace;
        db->debug_trace = NULL;
    }

    delete baton;
}

void Database::TraceCallback(void* db, const char* sql) {
    // Note: This function is called in the thread pool.
    // Note: Some queries, such as "EXPLAIN" queries, are not sent through this.
    static_cast<Database*>(db)->debug_trace->send(std::string(sql));
}

void Database::TraceCallback(EV_P_ ev_async *w, int revents) {
    // Note: This function is called in the main V8 thread.
    HandleScope scope;
    AsyncTrace* async = static_cast<AsyncTrace*>(w->data);

    std::vector<std::string> queries = async->get();
    for (unsigned int i = 0; i < queries.size(); i++) {
        Local<Value> argv[] = {
            String::NewSymbol("trace"),
            String::New(queries[i].c_str())
        };
        EMIT_EVENT(async->parent->handle_, 2, argv);
    }
    queries.clear();
}

Handle<Value> Database::Exec(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    REQUIRE_ARGUMENT_STRING(0, sql);
    OPTIONAL_ARGUMENT_FUNCTION(1, callback);

    Baton* baton = new ExecBaton(db, callback, *sql);
    db->Schedule(EIO_BeginExec, baton, true);

    return args.This();
}

void Database::EIO_BeginExec(Baton* baton) {
    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->handle);
    assert(baton->db->pending == 0);
    eio_custom(EIO_Exec, EIO_PRI_DEFAULT, EIO_AfterExec, baton);
}

int Database::EIO_Exec(eio_req *req) {
    ExecBaton* baton = static_cast<ExecBaton*>(req->data);

    char* message = NULL;
    baton->status = sqlite3_exec(
        baton->db->handle,
        baton->sql.c_str(),
        NULL,
        NULL,
        &message
    );

    if (baton->status != SQLITE_OK && message != NULL) {
        baton->message = std::string(message);
        sqlite3_free(message);
    }

    return 0;
}

int Database::EIO_AfterExec(eio_req *req) {
    HandleScope scope;
    ExecBaton* baton = static_cast<ExecBaton*>(req->data);
    Database* db = baton->db;


    if (baton->status != SQLITE_OK) {
        EXCEPTION(String::New(baton->message.c_str()), baton->status, exception);

        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            Local<Value> argv[] = { exception };
            TRY_CATCH_CALL(db->handle_, baton->callback, 1, argv);
        }
        else {
            Local<Value> args[] = { String::NewSymbol("error"), exception };
            EMIT_EVENT(db->handle_, 2, args);
        }
    }
    else if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
        Local<Value> argv[] = { Local<Value>::New(Null()) };
        TRY_CATCH_CALL(db->handle_, baton->callback, 1, argv);
    }

    db->Process();

    delete baton;
    return 0;
}

Handle<Value> Database::LoadExtension(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    REQUIRE_ARGUMENT_STRING(0, filename);
    OPTIONAL_ARGUMENT_FUNCTION(1, callback);

    Baton* baton = new LoadExtensionBaton(db, callback, *filename);
    db->Schedule(EIO_BeginLoadExtension, baton, true);

    return args.This();
}

void Database::EIO_BeginLoadExtension(Baton* baton) {
    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->handle);
    assert(baton->db->pending == 0);
    eio_custom(EIO_LoadExtension, EIO_PRI_DEFAULT, EIO_AfterLoadExtension, baton);
}

int Database::EIO_LoadExtension(eio_req *req) {
    LoadExtensionBaton* baton = static_cast<LoadExtensionBaton*>(req->data);

    sqlite3_enable_load_extension(baton->db->handle, 1);

    char* message = NULL;
    baton->status = sqlite3_load_extension(
        baton->db->handle,
        baton->filename.c_str(),
        0,
        &message
    );

    sqlite3_enable_load_extension(baton->db->handle, 0);

    if (baton->status != SQLITE_OK && message != NULL) {
        baton->message = std::string(message);
        sqlite3_free(message);
    }

    return 0;
}

int Database::EIO_AfterLoadExtension(eio_req *req) {
    HandleScope scope;
    LoadExtensionBaton* baton = static_cast<LoadExtensionBaton*>(req->data);
    Database* db = baton->db;

    if (baton->status != SQLITE_OK) {
        EXCEPTION(String::New(baton->message.c_str()), baton->status, exception);

        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            Local<Value> argv[] = { exception };
            TRY_CATCH_CALL(db->handle_, baton->callback, 1, argv);
        }
        else {
            Local<Value> args[] = { String::NewSymbol("error"), exception };
            EMIT_EVENT(db->handle_, 2, args);
        }
    }
    else if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
        Local<Value> argv[] = { Local<Value>::New(Null()) };
        TRY_CATCH_CALL(db->handle_, baton->callback, 1, argv);
    }

    db->Process();

    delete baton;
    return 0;
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

inline void Database::MakeWeak (void) {
    handle_.MakeWeak(this, Destruct);
}

void Database::Unref() {
    assert(!handle_.IsEmpty());
    assert(!handle_.IsWeak());
    assert(refs_ > 0);
    if (--refs_ == 0) { MakeWeak(); }
}

void Database::Destruct(Persistent<Value> value, void *data) {
    Database* db = static_cast<Database*>(data);

    if (db->debug_trace) {
        delete db->debug_trace;
        db->debug_trace = NULL;
    }

    if (db->handle) {
        eio_custom(EIO_Destruct, EIO_PRI_DEFAULT, EIO_AfterDestruct, db);
        ev_ref(EV_DEFAULT_UC);
    }
    else {
        delete db;
    }
}

int Database::EIO_Destruct(eio_req *req) {
    Database* db = static_cast<Database*>(req->data);

    sqlite3_close(db->handle);
    db->handle = NULL;

    return 0;
}

int Database::EIO_AfterDestruct(eio_req *req) {
    Database* db = static_cast<Database*>(req->data);
    ev_unref(EV_DEFAULT_UC);
    delete db;
    return 0;
}
