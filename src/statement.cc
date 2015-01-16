#include <string.h>
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

#include "macros.h"
#include "database.h"
#include "statement.h"

using namespace node_sqlite3;

Persistent<FunctionTemplate> Statement::constructor_template;

void Statement::Init(Handle<Object> target) {
    NanScope();

    Local<FunctionTemplate> t = NanNew<FunctionTemplate>(New);

    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(NanNew("Statement"));

    NODE_SET_PROTOTYPE_METHOD(t, "bind", Bind);
    NODE_SET_PROTOTYPE_METHOD(t, "get", Get);
    NODE_SET_PROTOTYPE_METHOD(t, "run", Run);
    NODE_SET_PROTOTYPE_METHOD(t, "all", All);
    NODE_SET_PROTOTYPE_METHOD(t, "each", Each);
    NODE_SET_PROTOTYPE_METHOD(t, "reset", Reset);
    NODE_SET_PROTOTYPE_METHOD(t, "finalize", Finalize);

    NanAssignPersistent(constructor_template, t);
    target->Set(NanNew("Statement"),
        t->GetFunction());
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

void Statement::Schedule(Work_Callback callback, Baton* baton) {
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
    NanScope();

    Statement* stmt = baton->stmt;
    // Fail hard on logic errors.
    assert(stmt->status != 0);
    EXCEPTION(NanNew<String>(stmt->message.c_str()), stmt->status, exception);

    Local<Function> cb = NanNew(baton->callback);

    if (!cb.IsEmpty() && cb->IsFunction()) {
        Local<Value> argv[] = { exception };
        TRY_CATCH_CALL(NanObjectWrapHandle(stmt), cb, 1, argv);
    }
    else {
        Local<Value> argv[] = { NanNew("error"), exception };
        EMIT_EVENT(NanObjectWrapHandle(stmt), 2, argv);
    }
}

// { Database db, String sql, Array params, Function callback }
NAN_METHOD(Statement::New) {
    NanScope();

    if (!args.IsConstructCall()) {
        return NanThrowTypeError("Use the new operator to create new Statement objects");
    }

    int length = args.Length();

    if (length <= 0 || !Database::HasInstance(args[0])) {
        return NanThrowTypeError("Database object expected");
    }
    else if (length <= 1 || !args[1]->IsString()) {
        return NanThrowTypeError("SQL query expected");
    }
    else if (length > 2 && !args[2]->IsUndefined() && !args[2]->IsFunction()) {
        return NanThrowTypeError("Callback expected");
    }

    Database* db = ObjectWrap::Unwrap<Database>(args[0]->ToObject());
    Local<String> sql = Local<String>::Cast(args[1]);

    args.This()->ForceSet(NanNew("sql"), sql, ReadOnly);

    Statement* stmt = new Statement(db);
    stmt->Wrap(args.This());

    PrepareBaton* baton = new PrepareBaton(db, Local<Function>::Cast(args[2]), stmt);
    baton->sql = std::string(*String::Utf8Value(sql));
    db->Schedule(Work_BeginPrepare, baton);

    NanReturnValue(args.This());
}

void Statement::Work_BeginPrepare(Database::Baton* baton) {
    assert(baton->db->open);
    baton->db->pending++;
    int status = uv_queue_work(uv_default_loop(),
        &baton->request, Work_Prepare, (uv_after_work_cb)Work_AfterPrepare);
    assert(status == 0);
}

void Statement::Work_Prepare(uv_work_t* req) {
    STATEMENT_INIT(PrepareBaton);

    // In case preparing fails, we use a mutex to make sure we get the associated
    // error message.
    sqlite3_mutex* mtx = sqlite3_db_mutex(baton->db->_handle);
    sqlite3_mutex_enter(mtx);

    stmt->status = sqlite3_prepare_v2(
        baton->db->_handle,
        baton->sql.c_str(),
        baton->sql.size(),
        &stmt->_handle,
        NULL
    );

    if (stmt->status != SQLITE_OK) {
        stmt->message = std::string(sqlite3_errmsg(baton->db->_handle));
        stmt->_handle = NULL;
    }

    sqlite3_mutex_leave(mtx);
}

void Statement::Work_AfterPrepare(uv_work_t* req) {
    NanScope();
    STATEMENT_INIT(PrepareBaton);

    if (stmt->status != SQLITE_OK) {
        Error(baton);
        stmt->Finalize();
    }
    else {
        stmt->prepared = true;
        Local<Function> cb = NanNew(baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            Local<Value> argv[] = { NanNew(NanNull()) };
            TRY_CATCH_CALL(NanObjectWrapHandle(stmt), cb, 1, argv);
        }
    }

    STATEMENT_END();
}

template <class T> Values::Field*
                   Statement::BindParameter(const Handle<Value> source, T pos) {
    if (source->IsString() || source->IsRegExp()) {
        String::Utf8Value val(source->ToString());
        return new Values::Text(pos, val.length(), *val);
    }
    else if (source->IsInt32()) {
        return new Values::Integer(pos, source->Int32Value());
    }
    else if (source->IsNumber()) {
        return new Values::Float(pos, source->NumberValue());
    }
    else if (source->IsBoolean()) {
        return new Values::Integer(pos, source->BooleanValue() ? 1 : 0);
    }
    else if (source->IsNull()) {
        return new Values::Null(pos);
    }
    else if (Buffer::HasInstance(source)) {
        Local<Object> buffer = source->ToObject();
        return new Values::Blob(pos, Buffer::Length(buffer), Buffer::Data(buffer));
    }
    else if (source->IsDate()) {
        return new Values::Float(pos, source->NumberValue());
    }
    else {
        return NULL;
    }
}

template <class T> T* Statement::Bind(_NAN_METHOD_ARGS, int start, int last) {
    NanScope();

    if (last < 0) last = args.Length();
    Local<Function> callback;
    if (last > start && args[last - 1]->IsFunction()) {
        callback = Local<Function>::Cast(args[last - 1]);
        last--;
    }

    T* baton = new T(this, callback);

    if (start < last) {
        if (args[start]->IsArray()) {
            Local<Array> array = Local<Array>::Cast(args[start]);
            int length = array->Length();
            // Note: bind parameters start with 1.
            for (int i = 0, pos = 1; i < length; i++, pos++) {
                baton->parameters.push_back(BindParameter(array->Get(i), pos));
            }
        }
        else if (!args[start]->IsObject() || args[start]->IsRegExp() || args[start]->IsDate() || Buffer::HasInstance(args[start])) {
            // Parameters directly in array.
            // Note: bind parameters start with 1.
            for (int i = start, pos = 1; i < last; i++, pos++) {
                baton->parameters.push_back(BindParameter(args[i], pos));
            }
        }
        else if (args[start]->IsObject()) {
            Local<Object> object = Local<Object>::Cast(args[start]);
            Local<Array> array = object->GetPropertyNames();
            int length = array->Length();
            for (int i = 0; i < length; i++) {
                Local<Value> name = array->Get(i);

                if (name->IsInt32()) {
                    baton->parameters.push_back(
                        BindParameter(object->Get(name), name->Int32Value()));
                }
                else {
                    baton->parameters.push_back(BindParameter(object->Get(name),
                        *String::Utf8Value(Local<String>::Cast(name))));
                }
            }
        }
        else {
            return NULL;
        }
    }

    return baton;
}

bool Statement::Bind(const Parameters & parameters) {
    if (parameters.size() == 0) {
        return true;
    }

    sqlite3_reset(_handle);
    sqlite3_clear_bindings(_handle);

    Parameters::const_iterator it = parameters.begin();
    Parameters::const_iterator end = parameters.end();

    for (; it < end; ++it) {
        Values::Field* field = *it;

        if (field != NULL) {
            int pos;
            if (field->index > 0) {
                pos = field->index;
            }
            else {
                pos = sqlite3_bind_parameter_index(_handle, field->name.c_str());
            }

            switch (field->type) {
                case SQLITE_INTEGER: {
                    status = sqlite3_bind_int(_handle, pos,
                        ((Values::Integer*)field)->value);
                } break;
                case SQLITE_FLOAT: {
                    status = sqlite3_bind_double(_handle, pos,
                        ((Values::Float*)field)->value);
                } break;
                case SQLITE_TEXT: {
                    status = sqlite3_bind_text(_handle, pos,
                        ((Values::Text*)field)->value.c_str(),
                        ((Values::Text*)field)->value.size(), SQLITE_TRANSIENT);
                } break;
                case SQLITE_BLOB: {
                    status = sqlite3_bind_blob(_handle, pos,
                        ((Values::Blob*)field)->value,
                        ((Values::Blob*)field)->length, SQLITE_TRANSIENT);
                } break;
                case SQLITE_NULL: {
                    status = sqlite3_bind_null(_handle, pos);
                } break;
            }

            if (status != SQLITE_OK) {
                message = std::string(sqlite3_errmsg(db->_handle));
                return false;
            }
        }
    }

    return true;
}

NAN_METHOD(Statement::Bind) {
    NanScope();
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    Baton* baton = stmt->Bind<Baton>(args);
    if (baton == NULL) {
        return NanThrowTypeError("Data type is not supported");
    }
    else {
        stmt->Schedule(Work_BeginBind, baton);
        NanReturnValue(args.This());
    }
}

void Statement::Work_BeginBind(Baton* baton) {
    STATEMENT_BEGIN(Bind);
}

void Statement::Work_Bind(uv_work_t* req) {
    STATEMENT_INIT(Baton);

    sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->_handle);
    sqlite3_mutex_enter(mtx);
    stmt->Bind(baton->parameters);
    sqlite3_mutex_leave(mtx);
}

void Statement::Work_AfterBind(uv_work_t* req) {
    NanScope();
    STATEMENT_INIT(Baton);

    if (stmt->status != SQLITE_OK) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        Local<Function> cb = NanNew(baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            Local<Value> argv[] = { NanNew(NanNull()) };
            TRY_CATCH_CALL(NanObjectWrapHandle(stmt), cb, 1, argv);
        }
    }

    STATEMENT_END();
}



NAN_METHOD(Statement::Get) {
    NanScope();
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    Baton* baton = stmt->Bind<RowBaton>(args);
    if (baton == NULL) {
        return NanThrowError("Data type is not supported");
    }
    else {
        stmt->Schedule(Work_BeginGet, baton);
        NanReturnValue(args.This());
    }
}

void Statement::Work_BeginGet(Baton* baton) {
    STATEMENT_BEGIN(Get);
}

void Statement::Work_Get(uv_work_t* req) {
    STATEMENT_INIT(RowBaton);

    if (stmt->status != SQLITE_DONE || baton->parameters.size()) {
        sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->_handle);
        sqlite3_mutex_enter(mtx);

        if (stmt->Bind(baton->parameters)) {
            stmt->status = sqlite3_step(stmt->_handle);

            if (!(stmt->status == SQLITE_ROW || stmt->status == SQLITE_DONE)) {
                stmt->message = std::string(sqlite3_errmsg(stmt->db->_handle));
            }
        }

        sqlite3_mutex_leave(mtx);

        if (stmt->status == SQLITE_ROW) {
            // Acquire one result row before returning.
            GetRow(&baton->row, stmt->_handle);
        }
    }
}

void Statement::Work_AfterGet(uv_work_t* req) {
    NanScope();
    STATEMENT_INIT(RowBaton);

    if (stmt->status != SQLITE_ROW && stmt->status != SQLITE_DONE) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        Local<Function> cb = NanNew(baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            if (stmt->status == SQLITE_ROW) {
                // Create the result array from the data we acquired.
                Local<Value> argv[] = { NanNew(NanNull()), RowToJS(&baton->row) };
                TRY_CATCH_CALL(NanObjectWrapHandle(stmt), cb, 2, argv);
            }
            else {
                Local<Value> argv[] = { NanNew(NanNull()) };
                TRY_CATCH_CALL(NanObjectWrapHandle(stmt), cb, 1, argv);
            }
        }
    }

    STATEMENT_END();
}

NAN_METHOD(Statement::Run) {
    NanScope();
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    Baton* baton = stmt->Bind<RunBaton>(args);
    if (baton == NULL) {
        return NanThrowError("Data type is not supported");
    }
    else {
        stmt->Schedule(Work_BeginRun, baton);
        NanReturnValue(args.This());
    }
}

void Statement::Work_BeginRun(Baton* baton) {
    STATEMENT_BEGIN(Run);
}

void Statement::Work_Run(uv_work_t* req) {
    STATEMENT_INIT(RunBaton);

    sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->_handle);
    sqlite3_mutex_enter(mtx);

    // Make sure that we also reset when there are no parameters.
    if (!baton->parameters.size()) {
        sqlite3_reset(stmt->_handle);
    }

    if (stmt->Bind(baton->parameters)) {
        stmt->status = sqlite3_step(stmt->_handle);

        if (!(stmt->status == SQLITE_ROW || stmt->status == SQLITE_DONE)) {
            stmt->message = std::string(sqlite3_errmsg(stmt->db->_handle));
        }
        else {
            baton->inserted_id = sqlite3_last_insert_rowid(stmt->db->_handle);
            baton->changes = sqlite3_changes(stmt->db->_handle);
        }
    }

    sqlite3_mutex_leave(mtx);
}

void Statement::Work_AfterRun(uv_work_t* req) {
    NanScope();
    STATEMENT_INIT(RunBaton);

    if (stmt->status != SQLITE_ROW && stmt->status != SQLITE_DONE) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        Local<Function> cb = NanNew(baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            NanObjectWrapHandle(stmt)->Set(NanNew("lastID"), NanNew<Number>(baton->inserted_id));
            NanObjectWrapHandle(stmt)->Set(NanNew("changes"), NanNew<Integer>(baton->changes));

            Local<Value> argv[] = { NanNew(NanNull()) };
            TRY_CATCH_CALL(NanObjectWrapHandle(stmt), cb, 1, argv);
        }
    }

    STATEMENT_END();
}

NAN_METHOD(Statement::All) {
    NanScope();
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    Baton* baton = stmt->Bind<RowsBaton>(args);
    if (baton == NULL) {
        return NanThrowError("Data type is not supported");
    }
    else {
        stmt->Schedule(Work_BeginAll, baton);
        NanReturnValue(args.This());
    }
}

void Statement::Work_BeginAll(Baton* baton) {
    STATEMENT_BEGIN(All);
}

void Statement::Work_All(uv_work_t* req) {
    STATEMENT_INIT(RowsBaton);

    sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->_handle);
    sqlite3_mutex_enter(mtx);

    // Make sure that we also reset when there are no parameters.
    if (!baton->parameters.size()) {
        sqlite3_reset(stmt->_handle);
    }

    if (stmt->Bind(baton->parameters)) {
        while ((stmt->status = sqlite3_step(stmt->_handle)) == SQLITE_ROW) {
            Row* row = new Row();
            GetRow(row, stmt->_handle);
            baton->rows.push_back(row);
        }

        if (stmt->status != SQLITE_DONE) {
            stmt->message = std::string(sqlite3_errmsg(stmt->db->_handle));
        }
    }

    sqlite3_mutex_leave(mtx);
}

void Statement::Work_AfterAll(uv_work_t* req) {
    NanScope();
    STATEMENT_INIT(RowsBaton);

    if (stmt->status != SQLITE_DONE) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        Local<Function> cb = NanNew(baton->callback);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            if (baton->rows.size()) {
                // Create the result array from the data we acquired.
                Local<Array> result(NanNew<Array>(baton->rows.size()));
                Rows::const_iterator it = baton->rows.begin();
                Rows::const_iterator end = baton->rows.end();
                for (int i = 0; it < end; ++it, i++) {
                    result->Set(i, RowToJS(*it));
                    delete *it;
                }

                Local<Value> argv[] = { NanNew(NanNull()), result };
                TRY_CATCH_CALL(NanObjectWrapHandle(stmt), cb, 2, argv);
            }
            else {
                // There were no result rows.
                Local<Value> argv[] = {
                    NanNew(NanNull()),
                    NanNew<Array>(0)
                };
                TRY_CATCH_CALL(NanObjectWrapHandle(stmt), cb, 2, argv);
            }
        }
    }

    STATEMENT_END();
}

NAN_METHOD(Statement::Each) {
    NanScope();
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    int last = args.Length();

    Local<Function> completed;
    if (last >= 2 && args[last - 1]->IsFunction() && args[last - 2]->IsFunction()) {
        completed = Local<Function>::Cast(args[--last]);
    }

    EachBaton* baton = stmt->Bind<EachBaton>(args, 0, last);
    if (baton == NULL) {
        return NanThrowError("Data type is not supported");
    }
    else {
        NanAssignPersistent(baton->completed, completed);
        stmt->Schedule(Work_BeginEach, baton);
        NanReturnValue(args.This());
    }
}

void Statement::Work_BeginEach(Baton* baton) {
    // Only create the Async object when we're actually going into
    // the event loop. This prevents dangling events.
    EachBaton* each_baton = static_cast<EachBaton*>(baton);
    each_baton->async = new Async(each_baton->stmt, reinterpret_cast<uv_async_cb>(AsyncEach));
    NanAssignPersistent(each_baton->async->item_cb, each_baton->callback);
    NanAssignPersistent(each_baton->async->completed_cb, each_baton->completed);

    STATEMENT_BEGIN(Each);
}

void Statement::Work_Each(uv_work_t* req) {
    STATEMENT_INIT(EachBaton);

    Async* async = baton->async;

    sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->_handle);

    int retrieved = 0;

    // Make sure that we also reset when there are no parameters.
    if (!baton->parameters.size()) {
        sqlite3_reset(stmt->_handle);
    }

    if (stmt->Bind(baton->parameters)) {
        while (true) {
            sqlite3_mutex_enter(mtx);
            stmt->status = sqlite3_step(stmt->_handle);
            if (stmt->status == SQLITE_ROW) {
                sqlite3_mutex_leave(mtx);
                Row* row = new Row();
                GetRow(row, stmt->_handle);
                NODE_SQLITE3_MUTEX_LOCK(&async->mutex)
                async->data.push_back(row);
                retrieved++;
                NODE_SQLITE3_MUTEX_UNLOCK(&async->mutex)

                uv_async_send(&async->watcher);
            }
            else {
                if (stmt->status != SQLITE_DONE) {
                    stmt->message = std::string(sqlite3_errmsg(stmt->db->_handle));
                }
                sqlite3_mutex_leave(mtx);
                break;
            }
        }
    }

    async->completed = true;
    uv_async_send(&async->watcher);
}

void Statement::CloseCallback(uv_handle_t* handle) {
    assert(handle != NULL);
    assert(handle->data != NULL);
    Async* async = static_cast<Async*>(handle->data);
    delete async;
}

void Statement::AsyncEach(uv_async_t* handle, int status) {
    NanScope();
    Async* async = static_cast<Async*>(handle->data);

    while (true) {
        // Get the contents out of the data cache for us to process in the JS callback.
        Rows rows;
        NODE_SQLITE3_MUTEX_LOCK(&async->mutex)
        rows.swap(async->data);
        NODE_SQLITE3_MUTEX_UNLOCK(&async->mutex)

        if (rows.empty()) {
            break;
        }

        Local<Function> cb = NanNew(async->item_cb);
        if (!cb.IsEmpty() && cb->IsFunction()) {
            Local<Value> argv[2];
            argv[0] = NanNew(NanNull());

            Rows::const_iterator it = rows.begin();
            Rows::const_iterator end = rows.end();
            for (int i = 0; it < end; ++it, i++) {
                argv[1] = RowToJS(*it);
                async->retrieved++;
                TRY_CATCH_CALL(NanObjectWrapHandle(async->stmt), cb, 2, argv);
                delete *it;
            }
        }
    }

    Local<Function> cb = NanNew(async->completed_cb);
    if (async->completed) {
        if (!cb.IsEmpty() &&
                cb->IsFunction()) {
            Local<Value> argv[] = {
                NanNew(NanNull()),
                NanNew<Integer>(async->retrieved)
            };
            TRY_CATCH_CALL(NanObjectWrapHandle(async->stmt), cb, 2, argv);
        }
        uv_close((uv_handle_t*)handle, CloseCallback);
    }
}

void Statement::Work_AfterEach(uv_work_t* req) {
    NanScope();
    STATEMENT_INIT(EachBaton);

    if (stmt->status != SQLITE_DONE) {
        Error(baton);
    }

    STATEMENT_END();
}

NAN_METHOD(Statement::Reset) {
    NanScope();
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(stmt, callback);
    stmt->Schedule(Work_BeginReset, baton);

    NanReturnValue(args.This());
}

void Statement::Work_BeginReset(Baton* baton) {
    STATEMENT_BEGIN(Reset);
}

void Statement::Work_Reset(uv_work_t* req) {
    STATEMENT_INIT(Baton);

    sqlite3_reset(stmt->_handle);
    stmt->status = SQLITE_OK;
}

void Statement::Work_AfterReset(uv_work_t* req) {
    NanScope();
    STATEMENT_INIT(Baton);

    // Fire callbacks.
    Local<Function> cb = NanNew(baton->callback);
    if (!cb.IsEmpty() && cb->IsFunction()) {
        Local<Value> argv[] = { NanNew(NanNull()) };
        TRY_CATCH_CALL(NanObjectWrapHandle(stmt), cb, 1, argv);
    }

    STATEMENT_END();
}

Local<Object> Statement::RowToJS(Row* row) {
    NanEscapableScope();

    Local<Object> result(NanNew<Object>());

    Row::const_iterator it = row->begin();
    Row::const_iterator end = row->end();
    for (int i = 0; it < end; ++it, i++) {
        Values::Field* field = *it;

        Local<Value> value;

        switch (field->type) {
            case SQLITE_INTEGER: {
                value = NanNew<Number>(((Values::Integer*)field)->value);
            } break;
            case SQLITE_FLOAT: {
                value = NanNew<Number>(((Values::Float*)field)->value);
            } break;
            case SQLITE_TEXT: {
                value = NanNew<String>(((Values::Text*)field)->value.c_str(), ((Values::Text*)field)->value.size());
            } break;
            case SQLITE_BLOB: {
                value = NanNew(NanNewBufferHandle(((Values::Blob*)field)->value, ((Values::Blob*)field)->length));
            } break;
            case SQLITE_NULL: {
                value = NanNew(NanNull());
            } break;
        }

        result->Set(NanNew(field->name.c_str()), value);

        DELETE_FIELD(field);
    }

    return NanEscapeScope(result);
}

void Statement::GetRow(Row* row, sqlite3_stmt* stmt) {
    int rows = sqlite3_column_count(stmt);

    for (int i = 0; i < rows; i++) {
        int type = sqlite3_column_type(stmt, i);
        const char* name = sqlite3_column_name(stmt, i);
        switch (type) {
            case SQLITE_INTEGER: {
                row->push_back(new Values::Integer(name, sqlite3_column_int64(stmt, i)));
            }   break;
            case SQLITE_FLOAT: {
                row->push_back(new Values::Float(name, sqlite3_column_double(stmt, i)));
            }   break;
            case SQLITE_TEXT: {
                const char* text = (const char*)sqlite3_column_text(stmt, i);
                int length = sqlite3_column_bytes(stmt, i);
                row->push_back(new Values::Text(name, length, text));
            } break;
            case SQLITE_BLOB: {
                const void* blob = sqlite3_column_blob(stmt, i);
                int length = sqlite3_column_bytes(stmt, i);
                row->push_back(new Values::Blob(name, length, blob));
            }   break;
            case SQLITE_NULL: {
                row->push_back(new Values::Null(name));
            }   break;
            default:
                assert(false);
        }
    }
}

NAN_METHOD(Statement::Finalize) {
    NanScope();
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());
    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(stmt, callback);
    stmt->Schedule(Finalize, baton);

    NanReturnValue(NanObjectWrapHandle(stmt->db));
}

void Statement::Finalize(Baton* baton) {
    NanScope();
    baton->stmt->Finalize();

    // Fire callback in case there was one.
    Local<Function> cb = NanNew(baton->callback);
    if (!cb.IsEmpty() && cb->IsFunction()) {
        TRY_CATCH_CALL(NanObjectWrapHandle(baton->stmt), cb, 0, NULL);
    }

    delete baton;
}

void Statement::Finalize() {
    assert(!finalized);
    finalized = true;
    CleanQueue();
    // Finalize returns the status code of the last operation. We already fired
    // error events in case those failed.
    sqlite3_finalize(_handle);
    _handle = NULL;
    db->Unref();
}

void Statement::CleanQueue() {
    NanScope();
    if (prepared && !queue.empty()) {
        // This statement has already been prepared and is now finalized.
        // Fire error for all remaining items in the queue.
        EXCEPTION(NanNew<String>("Statement is already finalized"), SQLITE_MISUSE, exception);
        Local<Value> argv[] = { exception };
        bool called = false;

        // Clear out the queue so that this object can get GC'ed.
        while (!queue.empty()) {
            Call* call = queue.front();
            queue.pop();

            Local<Function> cb = NanNew(call->baton->callback);

            if (prepared && !cb.IsEmpty() &&
                cb->IsFunction()) {
                TRY_CATCH_CALL(NanObjectWrapHandle(this), cb, 1, argv);
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
            Local<Value> args[] = { NanNew("error"), exception };
            EMIT_EVENT(NanObjectWrapHandle(this), 2, args);
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
