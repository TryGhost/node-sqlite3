#include <string.h>

#include "macros.h"
#include "database.h"
#include "statement.h"

using namespace node_sqlite3;

Napi::FunctionReference Database::constructor;

void Database::Init(Napi::Env env, Napi::Object exports, Napi::Object module) {
    Napi::HandleScope scope(env);

    Local<Napi::FunctionReference> t = Napi::Napi::FunctionReference::New(env, New);


    t->SetClassName(Napi::String::New(env, "Database"));

      InstanceMethod("close", &Close),
      InstanceMethod("exec", &Exec),
      InstanceMethod("wait", &Wait),
      InstanceMethod("loadExtension", &LoadExtension),
      InstanceMethod("serialize", &Serialize),
      InstanceMethod("parallelize", &Parallelize),
      InstanceMethod("configure", &Configure),
      InstanceMethod("interrupt", &Interrupt),

    NODE_SET_GETTER(t, "open", OpenGetter);

    constructor.Reset(t);

    (exports).Set( Napi::String::New(env, "Database"),
        Napi::GetFunction(t));
}

void Database::Process() {
    Napi::HandleScope scope(env);

    if (!open && locked && !queue.empty()) {
        EXCEPTION(Napi::String::New(env, "Database handle is closed"), SQLITE_MISUSE, exception);
        Napi::Value argv[] = { exception };
        bool called = false;

        // Call all callbacks with the error object.
        while (!queue.empty()) {
            Call* call = queue.front();
            Napi::Function cb = Napi::New(env, call->baton->callback);
            if (!cb.IsEmpty() && cb->IsFunction()) {
                TRY_CATCH_CALL(this->handle(), cb, 1, argv);
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
            Napi::Value info[] = { Napi::String::New(env, "error"), exception };
            EMIT_EVENT(handle(), 2, info);
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
    Napi::HandleScope scope(env);

    if (!open && locked) {
        EXCEPTION(Napi::String::New(env, "Database is closed"), SQLITE_MISUSE, exception);
        Napi::Function cb = Napi::New(env, baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            Napi::Value argv[] = { exception };
            TRY_CATCH_CALL(handle(), cb, 1, argv);
        }
        else {
            Napi::Value argv[] = { Napi::String::New(env, "error"), exception };
            EMIT_EVENT(handle(), 2, argv);
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

Napi::Value Database::New(const Napi::CallbackInfo& info) {
    if (!info.IsConstructCall()) {
        return Napi::TypeError::New(env, "Use the new operator to create new Database objects").ThrowAsJavaScriptException();
    }

    REQUIRE_ARGUMENT_STRING(0, filename);
    int pos = 1;

    int mode;
    if (info.Length() >= pos && info[pos]-.IsNumber()) {
        mode = info[pos++].As<Napi::Number>().Int32Value();
    } else {
        mode = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    }

    Napi::Function callback;
    if (info.Length() >= pos && info[pos].IsFunction()) {
        callback = Napi::Function::Cast(info[pos++]);
    }

    Database* db = new Database();
    db->Wrap(info.This());

    info.This().ForceSet(Napi::String::New(env, "filename"), info[0].As<Napi::String>(), ReadOnly);
    info.This().ForceSet(Napi::String::New(env, "mode"), Napi::New(env, mode), ReadOnly);

    // Start opening the database.
    OpenBaton* baton = new OpenBaton(db, callback, *filename, mode);
    Work_BeginOpen(baton);

    return info.This();
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
        &db->_handle,
        baton->mode,
        NULL
    );

    if (baton->status != SQLITE_OK) {
        baton->message = std::string(sqlite3_errmsg(db->_handle));
        sqlite3_close(db->_handle);
        db->_handle = NULL;
    }
    else {
        // Set default database handle values.
        sqlite3_busy_timeout(db->_handle, 1000);
    }
}

void Database::Work_AfterOpen(uv_work_t* req) {
    Napi::HandleScope scope(env);

    OpenBaton* baton = static_cast<OpenBaton*>(req->data);
    Database* db = baton->db;

    Napi::Value argv[1];
    if (baton->status != SQLITE_OK) {
        EXCEPTION(Napi::New(env, baton->message.c_str()), baton->status, exception);
        argv[0] = exception;
    }
    else {
        db->open = true;
        argv[0] = env.Null();
    }

    Napi::Function cb = Napi::New(env, baton->callback);

    if (!cb.IsEmpty() && cb->IsFunction()) {
        TRY_CATCH_CALL(db->handle(), cb, 1, argv);
    }
    else if (!db->open) {
        Napi::Value info[] = { Napi::String::New(env, "error"), argv[0] };
        EMIT_EVENT(db->handle(), 2, info);
    }

    if (db->open) {
        Napi::Value info[] = { Napi::String::New(env, "open") };
        EMIT_EVENT(db->handle(), 1, info);
        db->Process();
    }

    delete baton;
}

Napi::Value Database::OpenGetter(const Napi::CallbackInfo& info) {
    Database* db = this;
    return db->open;
}

Napi::Value Database::Close(const Napi::CallbackInfo& info) {
    Database* db = this;
    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(db, callback);
    db->Schedule(Work_BeginClose, baton, true);

    return info.This();
}

void Database::Work_BeginClose(Baton* baton) {
    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->_handle);
    assert(baton->db->pending == 0);

    baton->db->RemoveCallbacks();
    baton->db->closing = true;

    int status = uv_queue_work(uv_default_loop(),
        &baton->request, Work_Close, (uv_after_work_cb)Work_AfterClose);
    assert(status == 0);
}

void Database::Work_Close(uv_work_t* req) {
    Baton* baton = static_cast<Baton*>(req->data);
    Database* db = baton->db;

    baton->status = sqlite3_close(db->_handle);

    if (baton->status != SQLITE_OK) {
        baton->message = std::string(sqlite3_errmsg(db->_handle));
    }
    else {
        db->_handle = NULL;
    }
}

void Database::Work_AfterClose(uv_work_t* req) {
    Napi::HandleScope scope(env);

    Baton* baton = static_cast<Baton*>(req->data);
    Database* db = baton->db;

    db->closing = false;

    Napi::Value argv[1];
    if (baton->status != SQLITE_OK) {
        EXCEPTION(Napi::New(env, baton->message.c_str()), baton->status, exception);
        argv[0] = exception;
    }
    else {
        db->open = false;
        // Leave db->locked to indicate that this db object has reached
        // the end of its life.
        argv[0] = env.Null();
    }

    Napi::Function cb = Napi::New(env, baton->callback);

    // Fire callbacks.
    if (!cb.IsEmpty() && cb->IsFunction()) {
        TRY_CATCH_CALL(db->handle(), cb, 1, argv);
    }
    else if (db->open) {
        Napi::Value info[] = { Napi::String::New(env, "error"), argv[0] };
        EMIT_EVENT(db->handle(), 2, info);
    }

    if (!db->open) {
        Napi::Value info[] = { Napi::String::New(env, "close"), argv[0] };
        EMIT_EVENT(db->handle(), 1, info);
        db->Process();
    }

    delete baton;
}

Napi::Value Database::Serialize(const Napi::CallbackInfo& info) {
    Database* db = this;
    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    bool before = db->serialize;
    db->serialize = true;

    if (!callback.IsEmpty() && callback->IsFunction()) {
        TRY_CATCH_CALL(info.This(), callback, 0, NULL);
        db->serialize = before;
    }

    db->Process();

    return info.This();
}

Napi::Value Database::Parallelize(const Napi::CallbackInfo& info) {
    Database* db = this;
    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    bool before = db->serialize;
    db->serialize = false;

    if (!callback.IsEmpty() && callback->IsFunction()) {
        TRY_CATCH_CALL(info.This(), callback, 0, NULL);
        db->serialize = before;
    }

    db->Process();

    return info.This();
}

Napi::Value Database::Configure(const Napi::CallbackInfo& info) {
    Database* db = this;

    REQUIRE_ARGUMENTS(2);

    if (info[0].Equals( Napi::String::New(env, "trace"))) {
        Napi::Function handle;
        Baton* baton = new Baton(db, handle);
        db->Schedule(RegisterTraceCallback, baton);
    }
    else if (info[0].Equals( Napi::String::New(env, "profile"))) {
        Napi::Function handle;
        Baton* baton = new Baton(db, handle);
        db->Schedule(RegisterProfileCallback, baton);
    }
    else if (info[0].Equals( Napi::String::New(env, "busyTimeout"))) {
        if (!info[1]-.IsNumber()) {
            return Napi::TypeError::New(env, "Value must be an integer").ThrowAsJavaScriptException();
        }
        Napi::Function handle;
        Baton* baton = new Baton(db, handle);
        baton->status = info[1].As<Napi::Number>().Int32Value();
        db->Schedule(SetBusyTimeout, baton);
    }
    else {
        return Napi::ThrowError(Exception::Error(String::Concat(
            info[0].To<Napi::String>(),
            Napi::String::New(env, " is not a valid configuration option")
        )));
    }

    db->Process();

    return info.This();
}

Napi::Value Database::Interrupt(const Napi::CallbackInfo& info) {
    Database* db = this;

    if (!db->open) {
        return Napi::Error::New(env, "Database is not open").ThrowAsJavaScriptException();
    }

    if (db->closing) {
        return Napi::Error::New(env, "Database is closing").ThrowAsJavaScriptException();
    }

    sqlite3_interrupt(db->_handle);
    return info.This();
}

void Database::SetBusyTimeout(Baton* baton) {
    assert(baton->db->open);
    assert(baton->db->_handle);

    // Abuse the status field for passing the timeout.
    sqlite3_busy_timeout(baton->db->_handle, baton->status);

    delete baton;
}

void Database::RegisterTraceCallback(Baton* baton) {
    assert(baton->db->open);
    assert(baton->db->_handle);
    Database* db = baton->db;

    if (db->debug_trace == NULL) {
        // Add it.
        db->debug_trace = new AsyncTrace(db, TraceCallback);
        sqlite3_trace(db->_handle, TraceCallback, db);
    }
    else {
        // Remove it.
        sqlite3_trace(db->_handle, NULL, NULL);
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
    Napi::HandleScope scope(env);

    Napi::Value argv[] = {
        Napi::String::New(env, "trace"),
        Napi::New(env, sql->c_str())
    };
    EMIT_EVENT(db->handle(), 2, argv);
    delete sql;
}

void Database::RegisterProfileCallback(Baton* baton) {
    assert(baton->db->open);
    assert(baton->db->_handle);
    Database* db = baton->db;

    if (db->debug_profile == NULL) {
        // Add it.
        db->debug_profile = new AsyncProfile(db, ProfileCallback);
        sqlite3_profile(db->_handle, ProfileCallback, db);
    }
    else {
        // Remove it.
        sqlite3_profile(db->_handle, NULL, NULL);
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
    Napi::HandleScope scope(env);

    Napi::Value argv[] = {
        Napi::String::New(env, "profile"),
        Napi::New(env, info->sql.c_str()),
        Napi::Number::New(env, (double)info->nsecs / 1000000.0)
    };
    EMIT_EVENT(db->handle(), 3, argv);
    delete info;
}

void Database::RegisterUpdateCallback(Baton* baton) {
    assert(baton->db->open);
    assert(baton->db->_handle);
    Database* db = baton->db;

    if (db->update_event == NULL) {
        // Add it.
        db->update_event = new AsyncUpdate(db, UpdateCallback);
        sqlite3_update_hook(db->_handle, UpdateCallback, db);
    }
    else {
        // Remove it.
        sqlite3_update_hook(db->_handle, NULL, NULL);
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
    Napi::HandleScope scope(env);

    Napi::Value argv[] = {
        Napi::New(env, sqlite_authorizer_string(info->type)),
        Napi::New(env, info->database.c_str()),
        Napi::New(env, info->table.c_str()),
        Napi::Number::New(env, info->rowid),
    };
    EMIT_EVENT(db->handle(), 4, argv);
    delete info;
}

Napi::Value Database::Exec(const Napi::CallbackInfo& info) {
    Database* db = this;

    REQUIRE_ARGUMENT_STRING(0, sql);
    OPTIONAL_ARGUMENT_FUNCTION(1, callback);

    Baton* baton = new ExecBaton(db, callback, *sql);
    db->Schedule(Work_BeginExec, baton, true);

    return info.This();
}

void Database::Work_BeginExec(Baton* baton) {
    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->_handle);
    assert(baton->db->pending == 0);
    int status = uv_queue_work(uv_default_loop(),
        &baton->request, Work_Exec, (uv_after_work_cb)Work_AfterExec);
    assert(status == 0);
}

void Database::Work_Exec(uv_work_t* req) {
    ExecBaton* baton = static_cast<ExecBaton*>(req->data);

    char* message = NULL;
    baton->status = sqlite3_exec(
        baton->db->_handle,
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
    Napi::HandleScope scope(env);

    ExecBaton* baton = static_cast<ExecBaton*>(req->data);
    Database* db = baton->db;

    Napi::Function cb = Napi::New(env, baton->callback);

    if (baton->status != SQLITE_OK) {
        EXCEPTION(Napi::New(env, baton->message.c_str()), baton->status, exception);

        if (!cb.IsEmpty() && cb->IsFunction()) {
            Napi::Value argv[] = { exception };
            TRY_CATCH_CALL(db->handle(), cb, 1, argv);
        }
        else {
            Napi::Value info[] = { Napi::String::New(env, "error"), exception };
            EMIT_EVENT(db->handle(), 2, info);
        }
    }
    else if (!cb.IsEmpty() && cb->IsFunction()) {
        Napi::Value argv[] = { env.Null() };
        TRY_CATCH_CALL(db->handle(), cb, 1, argv);
    }

    db->Process();

    delete baton;
}

Napi::Value Database::Wait(const Napi::CallbackInfo& info) {
    Database* db = this;

    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(db, callback);
    db->Schedule(Work_Wait, baton, true);

    return info.This();
}

void Database::Work_Wait(Baton* baton) {
    Napi::HandleScope scope(env);

    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->_handle);
    assert(baton->db->pending == 0);

    Napi::Function cb = Napi::New(env, baton->callback);
    if (!cb.IsEmpty() && cb->IsFunction()) {
        Napi::Value argv[] = { env.Null() };
        TRY_CATCH_CALL(baton->db->handle(), cb, 1, argv);
    }

    baton->db->Process();

    delete baton;
}

Napi::Value Database::LoadExtension(const Napi::CallbackInfo& info) {
    Database* db = this;

    REQUIRE_ARGUMENT_STRING(0, filename);
    OPTIONAL_ARGUMENT_FUNCTION(1, callback);

    Baton* baton = new LoadExtensionBaton(db, callback, *filename);
    db->Schedule(Work_BeginLoadExtension, baton, true);

    return info.This();
}

void Database::Work_BeginLoadExtension(Baton* baton) {
    assert(baton->db->locked);
    assert(baton->db->open);
    assert(baton->db->_handle);
    assert(baton->db->pending == 0);
    int status = uv_queue_work(uv_default_loop(),
        &baton->request, Work_LoadExtension, reinterpret_cast<uv_after_work_cb>(Work_AfterLoadExtension));
    assert(status == 0);
}

void Database::Work_LoadExtension(uv_work_t* req) {
    LoadExtensionBaton* baton = static_cast<LoadExtensionBaton*>(req->data);

    sqlite3_enable_load_extension(baton->db->_handle, 1);

    char* message = NULL;
    baton->status = sqlite3_load_extension(
        baton->db->_handle,
        baton->filename.c_str(),
        0,
        &message
    );

    sqlite3_enable_load_extension(baton->db->_handle, 0);

    if (baton->status != SQLITE_OK && message != NULL) {
        baton->message = std::string(message);
        sqlite3_free(message);
    }
}

void Database::Work_AfterLoadExtension(uv_work_t* req) {
    Napi::HandleScope scope(env);

    LoadExtensionBaton* baton = static_cast<LoadExtensionBaton*>(req->data);
    Database* db = baton->db;
    Napi::Function cb = Napi::New(env, baton->callback);

    if (baton->status != SQLITE_OK) {
        EXCEPTION(Napi::New(env, baton->message.c_str()), baton->status, exception);

        if (!cb.IsEmpty() && cb->IsFunction()) {
            Napi::Value argv[] = { exception };
            TRY_CATCH_CALL(db->handle(), cb, 1, argv);
        }
        else {
            Napi::Value info[] = { Napi::String::New(env, "error"), exception };
            EMIT_EVENT(db->handle(), 2, info);
        }
    }
    else if (!cb.IsEmpty() && cb->IsFunction()) {
        Napi::Value argv[] = { env.Null() };
        TRY_CATCH_CALL(db->handle(), cb, 1, argv);
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
