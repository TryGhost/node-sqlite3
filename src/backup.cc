#include <string.h>
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

#include "macros.h"
#include "database.h"
#include "backup.h"

using namespace node_sqlite3;

Nan::Persistent<FunctionTemplate> Backup::constructor_template;


NAN_MODULE_INIT(Backup::Init) {
    Nan::HandleScope scope;

    Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(New);

    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(Nan::New("Backup").ToLocalChecked());

    Nan::SetPrototypeMethod(t, "step", Step);
    Nan::SetPrototypeMethod(t, "finish", Finish);

    NODE_SET_GETTER(t, "idle", IdleGetter);
    NODE_SET_GETTER(t, "completed", CompletedGetter);
    NODE_SET_GETTER(t, "failed", FailedGetter);
    NODE_SET_GETTER(t, "remaining", RemainingGetter);
    NODE_SET_GETTER(t, "pageCount", PageCountGetter);

    NODE_SET_SETTER(t, "retryErrors", RetryErrorGetter, RetryErrorSetter);

    constructor_template.Reset(t);
    Nan::Set(target, Nan::New("Backup").ToLocalChecked(),
        Nan::GetFunction(t).ToLocalChecked());
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
    Nan::HandleScope scope;

    Backup* backup = baton->backup;
    // Fail hard on logic errors.
    assert(backup->status != 0);
    EXCEPTION(backup->message, backup->status, exception);

    Local<Function> cb = Nan::New(baton->callback);

    if (!cb.IsEmpty() && cb->IsFunction()) {
        Local<Value> argv[] = { exception };
        TRY_CATCH_CALL(backup->handle(), cb, 1, argv);
    }
    else {
        Local<Value> argv[] = { Nan::New("error").ToLocalChecked(), exception };
        EMIT_EVENT(backup->handle(), 2, argv);
    }
}

void Backup::CleanQueue() {
    Nan::HandleScope scope;

    if (inited && !queue.empty()) {
        // This backup has already been initialized and is now finished.
        // Fire error for all remaining items in the queue.
        EXCEPTION("Backup is already finished", SQLITE_MISUSE, exception);
        Local<Value> argv[] = { exception };
        bool called = false;

        // Clear out the queue so that this object can get GC'ed.
        while (!queue.empty()) {
            Call* call = queue.front();
            queue.pop();

            Local<Function> cb = Nan::New(call->baton->callback);

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
            Local<Value> info[] = { Nan::New("error").ToLocalChecked(), exception };
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

NAN_METHOD(Backup::New) {
    if (!info.IsConstructCall()) {
        return Nan::ThrowTypeError("Use the new operator to create new Backup objects");
    }

    int length = info.Length();

    if (length <= 0 || !Database::HasInstance(info[0])) {
        return Nan::ThrowTypeError("Database object expected");
    }
    else if (length <= 1 || !info[1]->IsString()) {
        return Nan::ThrowTypeError("Filename expected");
    }
    else if (length <= 2 || !info[2]->IsString()) {
        return Nan::ThrowTypeError("Source database name expected");
    }
    else if (length <= 3 || !info[3]->IsString()) {
        return Nan::ThrowTypeError("Destination database name expected");
    }
    else if (length <= 4 || !info[4]->IsBoolean()) {
        return Nan::ThrowTypeError("Direction flag expected");
    }
    else if (length > 5 && !info[5]->IsUndefined() && !info[5]->IsFunction()) {
        return Nan::ThrowTypeError("Callback expected");
    }

    Database* db = Nan::ObjectWrap::Unwrap<Database>(info[0].As<Object>());
    Local<String> filename = Local<String>::Cast(info[1]);
    Local<String> sourceName = Local<String>::Cast(info[2]);
    Local<String> destName = Local<String>::Cast(info[3]);
    Local<Boolean> filenameIsDest = Local<Boolean>::Cast(info[4]);

    Nan::ForceSet(info.This(), Nan::New("filename").ToLocalChecked(), filename, ReadOnly);
    Nan::ForceSet(info.This(), Nan::New("sourceName").ToLocalChecked(), sourceName, ReadOnly);
    Nan::ForceSet(info.This(), Nan::New("destName").ToLocalChecked(), destName, ReadOnly);
    Nan::ForceSet(info.This(), Nan::New("filenameIsDest").ToLocalChecked(), filenameIsDest, ReadOnly);

    Backup* backup = new Backup(db);
    backup->Wrap(info.This());

    InitializeBaton* baton = new InitializeBaton(db, Local<Function>::Cast(info[5]), backup);
    baton->filename = std::string(*Nan::Utf8String(filename));
    baton->sourceName = std::string(*Nan::Utf8String(sourceName));
    baton->destName = std::string(*Nan::Utf8String(destName));
    baton->filenameIsDest = Nan::To<bool>(filenameIsDest).FromJust();
    db->Schedule(Work_BeginInitialize, baton);

    info.GetReturnValue().Set(info.This());
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
    Nan::HandleScope scope;

    BACKUP_INIT(InitializeBaton);

    if (backup->status != SQLITE_OK) {
        Error(baton);
        backup->FinishAll();
    }
    else {
        backup->inited = true;
        Local<Function> cb = Nan::New(baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            Local<Value> argv[] = { Nan::Null() };
            TRY_CATCH_CALL(backup->handle(), cb, 1, argv);
        }
    }
    BACKUP_END();
}

NAN_METHOD(Backup::Step) {
    Backup* backup = Nan::ObjectWrap::Unwrap<Backup>(info.This());

    REQUIRE_ARGUMENT_INTEGER(0, pages);
    OPTIONAL_ARGUMENT_FUNCTION(1, callback);

    StepBaton* baton = new StepBaton(backup, callback, pages);
    backup->GetRetryErrors(baton->retryErrorsSet);
    backup->Schedule(Work_BeginStep, baton);
    info.GetReturnValue().Set(info.This());
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
    Nan::HandleScope scope;

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
        Local<Function> cb = Nan::New(baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            Local<Value> argv[] = { Nan::Null(), Nan::New(backup->status == SQLITE_DONE) };
            TRY_CATCH_CALL(backup->handle(), cb, 2, argv);
        }
    }

    BACKUP_END();
}

NAN_METHOD(Backup::Finish) {
    Backup* backup = Nan::ObjectWrap::Unwrap<Backup>(info.This());

    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(backup, callback);
    backup->Schedule(Work_BeginFinish, baton);
    info.GetReturnValue().Set(info.This());
}

void Backup::Work_BeginFinish(Baton* baton) {
    BACKUP_BEGIN(Finish);
}

void Backup::Work_Finish(uv_work_t* req) {
    BACKUP_INIT(Baton);
    backup->FinishSqlite();
}

void Backup::Work_AfterFinish(uv_work_t* req) {
    Nan::HandleScope scope;

    BACKUP_INIT(Baton);
    backup->FinishAll();

    // Fire callback in case there was one.
    Local<Function> cb = Nan::New(baton->callback);
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

NAN_GETTER(Backup::IdleGetter) {
    Backup* backup = Nan::ObjectWrap::Unwrap<Backup>(info.This());
    bool idle = backup->inited && !backup->locked && backup->queue.empty();
    info.GetReturnValue().Set(idle);
}

NAN_GETTER(Backup::CompletedGetter) {
    Backup* backup = Nan::ObjectWrap::Unwrap<Backup>(info.This());
    info.GetReturnValue().Set(backup->completed);
}

NAN_GETTER(Backup::FailedGetter) {
    Backup* backup = Nan::ObjectWrap::Unwrap<Backup>(info.This());
    info.GetReturnValue().Set(backup->failed);
}

NAN_GETTER(Backup::RemainingGetter) {
    Backup* backup = Nan::ObjectWrap::Unwrap<Backup>(info.This());
    info.GetReturnValue().Set(backup->remaining);
}

NAN_GETTER(Backup::PageCountGetter) {
    Backup* backup = Nan::ObjectWrap::Unwrap<Backup>(info.This());
    info.GetReturnValue().Set(backup->pageCount);
}

NAN_GETTER(Backup::RetryErrorGetter) {
    Backup* backup = Nan::ObjectWrap::Unwrap<Backup>(info.This());
    info.GetReturnValue().Set(Nan::New(backup->retryErrors));
}

NAN_SETTER(Backup::RetryErrorSetter) {
    Backup* backup = Nan::ObjectWrap::Unwrap<Backup>(info.This());
    if (!value->IsArray()) {
        return Nan::ThrowError("retryErrors must be an array");
    }
    Local<Array> array = Local<Array>::Cast(value);
    backup->retryErrors.Reset(array);
}

void Backup::GetRetryErrors(std::set<int>& retryErrorsSet) {
    retryErrorsSet.clear();
    Local<Array> array = Nan::New(retryErrors);
    int length = array->Length();
    for (int i = 0; i < length; i++) {
        Local<Value> code = Nan::Get(array, i).ToLocalChecked();
        if (code->IsInt32()) {
            retryErrorsSet.insert(Nan::To<int32_t>(code).FromJust());
        }
    }
}

