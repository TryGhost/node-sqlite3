#include <string.h>
#include <napi.h>
#include <uv.h>
#include <node_buffer.h>
#include <node_version.h>

#include "macros.h"
#include "database.h"
#include "backup.h"

using namespace node_sqlite3;

Napi::FunctionReference Backup::constructor;


Napi::Object Backup::Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::FunctionReference t = Napi::Function::New(env, New);


    t->SetClassName(Napi::String::New(env, "Backup"));

    InstanceMethod("step", &Step),
    InstanceMethod("finish", &Finish),

    NODE_SET_GETTER(t, "idle", IdleGetter);
    NODE_SET_GETTER(t, "completed", CompletedGetter);
    NODE_SET_GETTER(t, "failed", FailedGetter);
    NODE_SET_GETTER(t, "remaining", RemainingGetter);
    NODE_SET_GETTER(t, "pageCount", PageCountGetter);

    NODE_SET_SETTER(t, "retryErrors", RetryErrorGetter, RetryErrorSetter);

    constructor.Reset(t);
    (target).Set(Napi::String::New(env, "Backup"),
        Napi::GetFunction(t));
}

void Backup::Process() {
    if (finished && !queue.empty()) {
        return CleanQueue();
    }

    while (inited && !locked && !queue.empty()) {
        Call* call = queue.front();
        queue.pop();

        call->callback(call->baton);
        delete call;
    }
}

void Backup::Schedule(Work_Callback callback, Baton* baton) {
    if (finished) {
        queue.push(new Call(callback, baton));
        CleanQueue();
    }
    else if (!inited || locked || !queue.empty()) {
        queue.push(new Call(callback, baton));
    }
    else {
        callback(baton);
    }
}

template <class T> void Backup::Error(T* baton) {
    Napi::HandleScope scope(env);

    Backup* backup = baton->backup;
    // Fail hard on logic errors.
    assert(backup->status != 0);
    EXCEPTION(backup->message, backup->status, exception);

    Napi::Function cb = Napi::New(env, baton->callback);

    if (!cb.IsEmpty() && cb->IsFunction()) {
        Napi::Value argv[] = { exception };
        TRY_CATCH_CALL(backup->handle(), cb, 1, argv);
    }
    else {
        Napi::Value argv[] = { Napi::String::New(env, "error"), exception };
        EMIT_EVENT(backup->handle(), 2, argv);
    }
}

void Backup::CleanQueue() {
    Napi::HandleScope scope(env);

    if (inited && !queue.empty()) {
        // This backup has already been initialized and is now finished.
        // Fire error for all remaining items in the queue.
        EXCEPTION("Backup is already finished", SQLITE_MISUSE, exception);
        Napi::Value argv[] = { exception };
        bool called = false;

        // Clear out the queue so that this object can get GC'ed.
        while (!queue.empty()) {
            Call* call = queue.front();
            queue.pop();

            Napi::Function cb = Napi::New(env, call->baton->callback);

            if (inited && !cb.IsEmpty() &&
                cb->IsFunction()) {
                TRY_CATCH_CALL(handle(), cb, 1, argv);
                called = true;
            }

            // We don't call the actual callback, so we have to make sure that
            // the baton gets destroyed.
            delete call->baton;
            delete call;
        }

        // When we couldn't call a callback function, emit an error on the
        // Backup object.
        if (!called) {
            Napi::Value info[] = { Napi::String::New(env, "error"), exception };
            EMIT_EVENT(handle(), 2, info);
        }
    }
    else while (!queue.empty()) {
        // Just delete all items in the queue; we already fired an event when
        // initializing the backup failed.
        Call* call = queue.front();
        queue.pop();

        // We don't call the actual callback, so we have to make sure that
        // the baton gets destroyed.
        delete call->baton;
        delete call;
    }
}

Napi::Value Backup::New(const Napi::CallbackInfo& info) {
    if (!info.IsConstructCall()) {
        Napi::TypeError::New(env, "Use the new operator to create new Backup objects").ThrowAsJavaScriptException();
        return env.Null();
    }

    int length = info.Length();

    if (length <= 0 || !Database::HasInstance(info[0])) {
        Napi::TypeError::New(env, "Database object expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    else if (length <= 1 || !info[1].IsString()) {
        Napi::TypeError::New(env, "Filename expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    else if (length <= 2 || !info[2].IsString()) {
        Napi::TypeError::New(env, "Source database name expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    else if (length <= 3 || !info[3].IsString()) {
        Napi::TypeError::New(env, "Destination database name expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    else if (length <= 4 || !info[4].IsBoolean()) {
        Napi::TypeError::New(env, "Direction flag expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    else if (length > 5 && !info[5].IsUndefined() && !info[5].IsFunction()) {
        Napi::TypeError::New(env, "Callback expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Database* db = info[0].As<Napi::Object>().Unwrap<Database>();
    Napi::String filename = info[1].As<Napi::String>();
    Napi::String sourceName = info[2].As<Napi::String>();
    Napi::String destName = info[3].As<Napi::String>();
    Napi::Boolean filenameIsDest = info[4].As<Napi::Boolean>();

    info.This().DefineProperty(Napi::String::New(env, "filename"), filename, ReadOnly);
    info.This().DefineProperty(Napi::String::New(env, "sourceName"), sourceName, ReadOnly);
    info.This().DefineProperty(Napi::String::New(env, "destName"), destName, ReadOnly);
    info.This().DefineProperty(Napi::String::New(env, "filenameIsDest"), filenameIsDest, ReadOnly);

    Backup* backup = new Backup(db);
    backup->Wrap(info.This());

    InitializeBaton* baton = new InitializeBaton(db, info[5].As<Napi::Function>(), backup);
    baton->filename = std::string(filename->As<Napi::String>().Utf8Value().c_str());
    baton->sourceName = std::string(sourceName->As<Napi::String>().Utf8Value().c_str());
    baton->destName = std::string(destName->As<Napi::String>().Utf8Value().c_str());
    baton->filenameIsDest = filenameIsDest.As<Napi::Boolean>().Value();
    db->Schedule(Work_BeginInitialize, baton);

    return info.This();
}

void Backup::Work_BeginInitialize(Database::Baton* baton) {
    assert(baton->db->open);
    baton->db->pending++;
    int status = uv_queue_work(uv_default_loop(),
        &baton->request, Work_Initialize, (uv_after_work_cb)Work_AfterInitialize);
    assert(status == 0);
}

void Backup::Work_Initialize(uv_work_t* req) {
    BACKUP_INIT(InitializeBaton);

    // In case stepping fails, we use a mutex to make sure we get the associated
    // error message.
    sqlite3_mutex* mtx = sqlite3_db_mutex(baton->db->_handle);
    sqlite3_mutex_enter(mtx);

    backup->status = sqlite3_open(baton->filename.c_str(), &backup->_otherDb);

    if (backup->status == SQLITE_OK) {
        backup->_handle = sqlite3_backup_init(
            baton->filenameIsDest ? backup->_otherDb : backup->db->_handle,
            baton->destName.c_str(),
            baton->filenameIsDest ? backup->db->_handle : backup->_otherDb,
            baton->sourceName.c_str());
    }
    backup->_destDb = baton->filenameIsDest ? backup->_otherDb : backup->db->_handle;

    if (backup->status != SQLITE_OK) {
        backup->message = std::string(sqlite3_errmsg(backup->_destDb));
        sqlite3_close(backup->_otherDb);
        backup->_otherDb = NULL;
        backup->_destDb = NULL;
    }

    sqlite3_mutex_leave(mtx);
}

void Backup::Work_AfterInitialize(uv_work_t* req) {
    Napi::HandleScope scope(env);

    BACKUP_INIT(InitializeBaton);

    if (backup->status != SQLITE_OK) {
        Error(baton);
        backup->FinishAll();
    }
    else {
        backup->inited = true;
        Napi::Function cb = Napi::New(env, baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            Napi::Value argv[] = { env.Null() };
            TRY_CATCH_CALL(backup->handle(), cb, 1, argv);
        }
    }
    BACKUP_END();
}

Napi::Value Backup::Step(const Napi::CallbackInfo& info) {
    Backup* backup = this;

    REQUIRE_ARGUMENT_INTEGER(0, pages);
    OPTIONAL_ARGUMENT_FUNCTION(1, callback);

    StepBaton* baton = new StepBaton(backup, callback, pages);
    backup->GetRetryErrors(baton->retryErrorsSet);
    backup->Schedule(Work_BeginStep, baton);
    return info.This();
}

void Backup::Work_BeginStep(Baton* baton) {
    BACKUP_BEGIN(Step);
}

void Backup::Work_Step(uv_work_t* req) {
    BACKUP_INIT(StepBaton);
    if (backup->_handle) {
        backup->status = sqlite3_backup_step(backup->_handle, baton->pages);
        backup->remaining = sqlite3_backup_remaining(backup->_handle);
        backup->pageCount = sqlite3_backup_pagecount(backup->_handle);
    }
    if (backup->status != SQLITE_OK) {
        // Text of message is a little awkward to get, since the error is not associated
        // with a db connection.
#if SQLITE_VERSION_NUMBER >= 3007015
        // sqlite3_errstr is a relatively new method
        backup->message = std::string(sqlite3_errstr(backup->status));
#else
        backup->message = "Sqlite error";
#endif
        if (baton->retryErrorsSet.size() > 0) {
            if (baton->retryErrorsSet.find(backup->status) == baton->retryErrorsSet.end()) {
                backup->FinishSqlite();
            }
        }
    }
}

void Backup::Work_AfterStep(uv_work_t* req) {
    Napi::HandleScope scope(env);

    BACKUP_INIT(StepBaton);

    if (backup->status == SQLITE_DONE) {
        backup->completed = true;
    } else if (!backup->_handle) {
        backup->failed = true;
    }

    if (backup->status != SQLITE_OK && backup->status != SQLITE_DONE) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        Napi::Function cb = Napi::New(env, baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            Napi::Value argv[] = { env.Null(), Napi::New(env, backup->status == SQLITE_DONE) };
            TRY_CATCH_CALL(backup->handle(), cb, 2, argv);
        }
    }

    BACKUP_END();
}

Napi::Value Backup::Finish(const Napi::CallbackInfo& info) {
    Backup* backup = this;

    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(backup, callback);
    backup->Schedule(Work_BeginFinish, baton);
    return info.This();
}

void Backup::Work_BeginFinish(Baton* baton) {
    BACKUP_BEGIN(Finish);
}

void Backup::Work_Finish(uv_work_t* req) {
    BACKUP_INIT(Baton);
    backup->FinishSqlite();
}

void Backup::Work_AfterFinish(uv_work_t* req) {
    Napi::HandleScope scope(env);

    BACKUP_INIT(Baton);
    backup->FinishAll();

    // Fire callback in case there was one.
    Napi::Function cb = Napi::New(env, baton->callback);
    if (!cb.IsEmpty() && cb->IsFunction()) {
        TRY_CATCH_CALL(backup->handle(), cb, 0, NULL);
    }

    BACKUP_END();
}

void Backup::FinishAll() {
    assert(!finished);
    if (!completed && !failed) {
        failed = true;
    }
    finished = true;
    CleanQueue();
    FinishSqlite();
    db->Unref();
}

void Backup::FinishSqlite() {
    if (_handle) {
        sqlite3_backup_finish(_handle);
        _handle = NULL;
    }
    if (_otherDb) {
        sqlite3_close(_otherDb);
        _otherDb = NULL;
    }
    _destDb = NULL;
}

Napi::Value Backup::IdleGetter(const Napi::CallbackInfo& info) {
    Backup* backup = this;
    bool idle = backup->inited && !backup->locked && backup->queue.empty();
    return idle;
}

Napi::Value Backup::CompletedGetter(const Napi::CallbackInfo& info) {
    Backup* backup = this;
    return backup->completed;
}

Napi::Value Backup::FailedGetter(const Napi::CallbackInfo& info) {
    Backup* backup = this;
    return backup->failed;
}

Napi::Value Backup::RemainingGetter(const Napi::CallbackInfo& info) {
    Backup* backup = this;
    return backup->remaining;
}

Napi::Value Backup::PageCountGetter(const Napi::CallbackInfo& info) {
    Backup* backup = this;
    return backup->pageCount;
}

Napi::Value Backup::RetryErrorGetter(const Napi::CallbackInfo& info) {
    Backup* backup = this;
    return Napi::New(env, backup->retryErrors);
}

void Backup::RetryErrorSetter(const Napi::CallbackInfo& info, const Napi::Value& value) {
    Backup* backup = this;
    if (!value->IsArray()) {
        Napi::Error::New(env, "retryErrors must be an array").ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::Array array = value.As<Napi::Array>();
    backup->retryErrors.Reset(array);
}

void Backup::GetRetryErrors(std::set<int>& retryErrorsSet) {
    retryErrorsSet.clear();
    Napi::Array array = Napi::New(env, retryErrors);
    int length = array->Length();
    for (int i = 0; i < length; i++) {
        Napi::Value code = (array).Get(i);
        if (code.IsNumber()) {
            retryErrorsSet.insert(code.As<Napi::Number>().Int32Value());
        }
    }
}

