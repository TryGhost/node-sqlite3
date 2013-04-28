#include <string.h>
#include <node.h>

#include "macros.h"
#include "database.h"
#include "statement.h"

using namespace node_sqlite3;

Persistent<FunctionTemplate> Database::constructor_template;

void Database::Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("Database"));

    NODE_SET_PROTOTYPE_METHOD(constructor_template, "close", Close);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "exec", Exec);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "wait", Wait);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "loadExtension", LoadExtension);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "serialize", Serialize);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "parallelize", Parallelize);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "configure", Configure);

    NODE_SET_GETTER(constructor_template, "open", OpenGetter);

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

void Database::Schedule(Work_Callback callback, Baton* baton, bool exclusive) {
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

    if (!args.IsConstructCall()) {
        return ThrowException(Exception::TypeError(
            String::New("Use the new operator to create new Database objects"))
        );
    }

    REQUIRE_ARGUMENT_STRING(0, filename);
    int pos = 1;

    int mode;
    if (args.Length() >= pos && args[pos]->IsInt32()) {
        mode = args[pos++]->Int32Value();
    } else {
        mode = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
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
    OpenBaton* baton = new OpenBaton(db, callback, *filename, mode);
    Work_BeginOpen(baton);

    return args.This();
}

void Database::Work_BeginOpen(Baton* baton) {
    int status = uv_queue_work(uv_default_loop(),
        &baton->request, Work_Open, (uv_after_work_cb)Work_AfterOpen);
    assert(status == 0);
}

void Database::Work_Open(uv_work_t* req) {
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
    else {
        // Set default database handle values.
        sqlite3_busy_timeout(db->handle, 1000);
    }
}

void Database::Work_AfterOpen(uv_work_t* req) {
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
}

Handle<Value> Database::OpenGetter(Local<String> str, const AccessorInfo& accessor) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(accessor.This());
    return Boolean::New(db->open);
}

Handle<Value> Database::Close(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());
    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(db, callback);
    db->Schedule(Work_BeginClose, baton, true);

    return args.This();
}

void Database::Work_BeginClose(Baton* baton) {
    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->handle);
    assert(baton->db->pending == 0);

    baton->db->RemoveCallbacks();
    int status = uv_queue_work(uv_default_loop(),
        &baton->request, Work_Close, (uv_after_work_cb)Work_AfterClose);
    assert(status == 0);
}

void Database::Work_Close(uv_work_t* req) {
    Baton* baton = static_cast<Baton*>(req->data);
    Database* db = baton->db;

    baton->status = sqlite3_close(db->handle);

    if (baton->status != SQLITE_OK) {
        baton->message = std::string(sqlite3_errmsg(db->handle));
    }
    else {
        db->handle = NULL;
    }
}

void Database::Work_AfterClose(uv_work_t* req) {
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
    else if (args[0]->Equals(String::NewSymbol("profile"))) {
        Local<Function> handle;
        Baton* baton = new Baton(db, handle);
        db->Schedule(RegisterProfileCallback, baton);
    }
    else if (args[0]->Equals(String::NewSymbol("busyTimeout"))) {
        if (!args[1]->IsInt32()) {
            return ThrowException(Exception::TypeError(
                String::New("Value must be an integer"))
            );
        }
        Local<Function> handle;
        Baton* baton = new Baton(db, handle);
        baton->status = args[1]->Int32Value();
        db->Schedule(SetBusyTimeout, baton);
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

void Database::SetBusyTimeout(Baton* baton) {
    assert(baton->db->open);
    assert(baton->db->handle);

    // Abuse the status field for passing the timeout.
    sqlite3_busy_timeout(baton->db->handle, baton->status);

    delete baton;
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
        db->debug_trace->finish();
        db->debug_trace = NULL;
    }

    delete baton;
}

void Database::TraceCallback(void* db, const char* sql) {
    // Note: This function is called in the thread pool.
    // Note: Some queries, such as "EXPLAIN" queries, are not sent through this.
    static_cast<Database*>(db)->debug_trace->send(new std::string(sql));
}

void Database::TraceCallback(Database* db, std::string* sql) {
    // Note: This function is called in the main V8 thread.
    HandleScope scope;
    Local<Value> argv[] = {
        String::NewSymbol("trace"),
        String::New(sql->c_str())
    };
    EMIT_EVENT(db->handle_, 2, argv);
    delete sql;
}

void Database::RegisterProfileCallback(Baton* baton) {
    assert(baton->db->open);
    assert(baton->db->handle);
    Database* db = baton->db;

    if (db->debug_profile == NULL) {
        // Add it.
        db->debug_profile = new AsyncProfile(db, ProfileCallback);
        sqlite3_profile(db->handle, ProfileCallback, db);
    }
    else {
        // Remove it.
        sqlite3_profile(db->handle, NULL, NULL);
        db->debug_profile->finish();
        db->debug_profile = NULL;
    }

    delete baton;
}

void Database::ProfileCallback(void* db, const char* sql, sqlite3_uint64 nsecs) {
    // Note: This function is called in the thread pool.
    // Note: Some queries, such as "EXPLAIN" queries, are not sent through this.
    ProfileInfo* info = new ProfileInfo();
    info->sql = std::string(sql);
    info->nsecs = nsecs;
    static_cast<Database*>(db)->debug_profile->send(info);
}

void Database::ProfileCallback(Database *db, ProfileInfo* info) {
    HandleScope scope;
    Local<Value> argv[] = {
        String::NewSymbol("profile"),
        String::New(info->sql.c_str()),
        Integer::New((double)info->nsecs / 1000000.0)
    };
    EMIT_EVENT(db->handle_, 3, argv);
    delete info;
}

void Database::RegisterUpdateCallback(Baton* baton) {
    assert(baton->db->open);
    assert(baton->db->handle);
    Database* db = baton->db;

    if (db->update_event == NULL) {
        // Add it.
        db->update_event = new AsyncUpdate(db, UpdateCallback);
        sqlite3_update_hook(db->handle, UpdateCallback, db);
    }
    else {
        // Remove it.
        sqlite3_update_hook(db->handle, NULL, NULL);
        db->update_event->finish();
        db->update_event = NULL;
    }

    delete baton;
}

void Database::UpdateCallback(void* db, int type, const char* database,
        const char* table, sqlite3_int64 rowid) {
    // Note: This function is called in the thread pool.
    // Note: Some queries, such as "EXPLAIN" queries, are not sent through this.
    UpdateInfo* info = new UpdateInfo();
    info->type = type;
    info->database = std::string(database);
    info->table = std::string(table);
    info->rowid = rowid;
    static_cast<Database*>(db)->update_event->send(info);
}

void Database::UpdateCallback(Database *db, UpdateInfo* info) {
    HandleScope scope;

    Local<Value> argv[] = {
        String::NewSymbol(sqlite_authorizer_string(info->type)),
        String::New(info->database.c_str()),
        String::New(info->table.c_str()),
        Integer::New(info->rowid),
    };
    EMIT_EVENT(db->handle_, 4, argv);
    delete info;
}

Handle<Value> Database::Exec(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    REQUIRE_ARGUMENT_STRING(0, sql);
    OPTIONAL_ARGUMENT_FUNCTION(1, callback);

    Baton* baton = new ExecBaton(db, callback, *sql);
    db->Schedule(Work_BeginExec, baton, true);

    return args.This();
}

void Database::Work_BeginExec(Baton* baton) {
    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->handle);
    assert(baton->db->pending == 0);
    int status = uv_queue_work(uv_default_loop(),
        &baton->request, Work_Exec, (uv_after_work_cb)Work_AfterExec);
    assert(status == 0);
}

void Database::Work_Exec(uv_work_t* req) {
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
}

void Database::Work_AfterExec(uv_work_t* req) {
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
}

Handle<Value> Database::Wait(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(db, callback);
    db->Schedule(Work_Wait, baton, true);

    return args.This();
}

void Database::Work_Wait(Baton* baton) {
    HandleScope scope;

    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->handle);
    assert(baton->db->pending == 0);

    if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
        Local<Value> argv[] = { Local<Value>::New(Null()) };
        TRY_CATCH_CALL(baton->db->handle_, baton->callback, 1, argv);
    }

    baton->db->Process();

    delete baton;
}

Handle<Value> Database::LoadExtension(const Arguments& args) {
    HandleScope scope;
    Database* db = ObjectWrap::Unwrap<Database>(args.This());

    REQUIRE_ARGUMENT_STRING(0, filename);
    OPTIONAL_ARGUMENT_FUNCTION(1, callback);

    Baton* baton = new LoadExtensionBaton(db, callback, *filename);
    db->Schedule(Work_BeginLoadExtension, baton, true);

    return args.This();
}

void Database::Work_BeginLoadExtension(Baton* baton) {
    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->handle);
    assert(baton->db->pending == 0);
    int status = uv_queue_work(uv_default_loop(),
        &baton->request, Work_LoadExtension, (uv_after_work_cb)Work_AfterLoadExtension);
    assert(status == 0);
}

void Database::Work_LoadExtension(uv_work_t* req) {
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
}

void Database::Work_AfterLoadExtension(uv_work_t* req) {
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
}

void Database::RemoveCallbacks() {
    if (debug_trace) {
        debug_trace->finish();
        debug_trace = NULL;
    }
    if (debug_profile) {
        debug_profile->finish();
        debug_profile = NULL;
    }
}
