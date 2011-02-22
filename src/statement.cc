#include <string.h>
#include <v8.h>
#include <node.h>
#include <node_events.h>

#include "macros.h"
#include "database.h"
#include "statement.h"
#include "deferred_call.h"

using namespace node_sqlite3;

Persistent<FunctionTemplate> Statement::constructor_template;

void Statement::Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(EventEmitter::constructor_template);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("Statement"));

    NODE_SET_PROTOTYPE_METHOD(constructor_template, "bind", Bind);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "get", Get);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "run", Run);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "all", All);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "reset", Reset);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "finalize", Finalize);

    target->Set(String::NewSymbol("Statement"),
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
    Statement* stmt = baton->stmt;
    EXCEPTION(String::New(stmt->message.c_str()), stmt->status, exception);

    if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
        Local<Value> argv[] = { exception };
        TRY_CATCH_CALL(stmt->handle_, baton->callback, 1, argv);
    }
    else {
        Local<Value> argv[] = { String::NewSymbol("error"), exception };
        EMIT_EVENT(stmt->handle_, 2, argv);
    }
}

// { Database db, String sql, Array params, Function callback }
Handle<Value> Statement::New(const Arguments& args) {
    HandleScope scope;

    int length = args.Length();

    if (length <= 0 || !Database::HasInstance(args[0])) {
        return ThrowException(Exception::TypeError(
            String::New("Database object expected")));
    }
    else if (length <= 1 || !args[1]->IsString()) {
        return ThrowException(Exception::TypeError(
            String::New("SQL query expected")));
    }
    else if (length > 2 && !args[2]->IsUndefined() && !args[2]->IsFunction()) {
        return ThrowException(Exception::TypeError(
            String::New("Callback expected")));
    }

    Database* db = ObjectWrap::Unwrap<Database>(args[0]->ToObject());
    Local<String> sql = Local<String>::Cast(args[1]);

    args.This()->Set(String::NewSymbol("sql"), Persistent<String>::New(sql), ReadOnly);

    Statement* stmt = new Statement(db);
    stmt->Wrap(args.This());
    PrepareBaton* baton = new PrepareBaton(db, Local<Function>::Cast(args[2]), stmt);
    baton->sql = std::string(*String::Utf8Value(sql));
    db->Schedule(EIO_BeginPrepare, baton);

    return args.This();
}


void Statement::EIO_BeginPrepare(Database::Baton* baton) {
    assert(baton->db->open);
    baton->db->pending++;
    eio_custom(EIO_Prepare, EIO_PRI_DEFAULT, EIO_AfterPrepare, baton);
}

int Statement::EIO_Prepare(eio_req *req) {
    STATEMENT_INIT(PrepareBaton);

    // In case preparing fails, we use a mutex to make sure we get the associated
    // error message.
    sqlite3_mutex* mtx = sqlite3_db_mutex(baton->db->handle);
    sqlite3_mutex_enter(mtx);

    stmt->status = sqlite3_prepare_v2(
        baton->db->handle,
        baton->sql.c_str(),
        baton->sql.size(),
        &stmt->handle,
        NULL
    );

    if (stmt->status != SQLITE_OK) {
        stmt->message = std::string(sqlite3_errmsg(baton->db->handle));
        stmt->handle = NULL;
    }

    sqlite3_mutex_leave(mtx);

    return 0;
}

int Statement::EIO_AfterPrepare(eio_req *req) {
    HandleScope scope;
    STATEMENT_INIT(PrepareBaton);

    if (stmt->status != SQLITE_OK) {
        Error(baton);
        stmt->Finalize();
    }
    else {
        stmt->prepared = true;
        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            Local<Value> argv[] = { Local<Value>::New(Null()) };
            TRY_CATCH_CALL(stmt->handle_, baton->callback, 1, argv);
        }
    }

    STATEMENT_END();
    return 0;
}

template <class T> T* Statement::Bind(const Arguments& args, int start) {
    int last = args.Length();
    Local<Function> callback;
    if (last > start && args[last - 1]->IsFunction()) {
        callback = Local<Function>::Cast(args[last - 1]);
        last--;
    }

    T* baton = new T(this, callback);

    for (int i = start; i < last; i++) {
        if (args[i]->IsString()) {
            String::Utf8Value val(args[i]->ToString());
            baton->parameters.push_back(new Data::Text(val.length(), *val));
        }
        else if (args[i]->IsInt32() || args[i]->IsUint32()) {
            baton->parameters.push_back(new Data::Integer(args[i]->Int32Value()));
        }
        else if (args[i]->IsNumber()) {
            baton->parameters.push_back(new Data::Float(args[i]->NumberValue()));
        }
        else if (args[i]->IsBoolean()) {
            baton->parameters.push_back(new Data::Integer(args[i]->BooleanValue() ? 1 : 0));
        }
        else if (args[i]->IsNull()) {
            baton->parameters.push_back(new Data::Null());
        }
        else if (args[i]->IsUndefined()) {
            // Skip parameter position.
            baton->parameters.push_back(NULL);
        }
        else {
            delete baton;
            return NULL;
        }
    }

    return baton;
}

bool Statement::Bind(const Data::Parameters parameters) {
    if (parameters.size() == 0) {
        return true;
    }

    sqlite3_reset(handle);
    sqlite3_clear_bindings(handle);

    Data::Parameters::const_iterator it = parameters.begin();
    Data::Parameters::const_iterator end = parameters.end();

    // Note: bind parameters start with 1.
    for (int i = 1; it < end; it++, i++) {
        Data::Field* field = *it;
        if (field == NULL) {
            continue;
        }

        switch (field->type) {
            case SQLITE_INTEGER: {
                status = sqlite3_bind_int(handle, i, ((Data::Integer*)field)->value);
            } break;
            case SQLITE_FLOAT: {
                status = sqlite3_bind_double(handle, i, ((Data::Float*)field)->value);
            } break;
            case SQLITE_TEXT: {
                status = sqlite3_bind_text(
                    handle, i, ((Data::Text*)field)->value.c_str(),
                    ((Data::Text*)field)->value.size(), SQLITE_TRANSIENT);
            } break;
            // case SQLITE_BLOB: {
            //
            // } break;
            case SQLITE_NULL: {
                status = sqlite3_bind_null(handle, i);
            } break;
        }

        if (status != SQLITE_OK) {
            message = std::string(sqlite3_errmsg(db->handle));
            return false;
        }
    }

    return true;
}

Handle<Value> Statement::Bind(const Arguments& args) {
    HandleScope scope;
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    Baton* baton = stmt->Bind<Baton>(args);
    if (baton == NULL) {
        return ThrowException(Exception::Error(String::New("Data type is not supported")));
    }
    else {
        stmt->Schedule(EIO_BeginBind, baton);
        return args.This();
    }
}

void Statement::EIO_BeginBind(Baton* baton) {
    STATEMENT_BEGIN(Bind);
}

int Statement::EIO_Bind(eio_req *req) {
    STATEMENT_INIT(Baton);

    sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->handle);
    sqlite3_mutex_enter(mtx);
    stmt->Bind(baton->parameters);
    sqlite3_mutex_leave(mtx);

    return 0;
}

int Statement::EIO_AfterBind(eio_req *req) {
    HandleScope scope;
    STATEMENT_INIT(Baton);

    if (stmt->status != SQLITE_OK) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            Local<Value> argv[] = { Local<Value>::New(Null()) };
            TRY_CATCH_CALL(stmt->handle_, baton->callback, 1, argv);
        }
    }

    STATEMENT_END();
    return 0;
}



Handle<Value> Statement::Get(const Arguments& args) {
    HandleScope scope;
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    Baton* baton = stmt->Bind<RowBaton>(args);
    stmt->Schedule(EIO_BeginGet, baton);

    return args.This();
}

void Statement::EIO_BeginGet(Baton* baton) {
    STATEMENT_BEGIN(Get);
}

int Statement::EIO_Get(eio_req *req) {
    STATEMENT_INIT(RowBaton);

    if (stmt->status != SQLITE_DONE) {
        sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->handle);
        sqlite3_mutex_enter(mtx);

        if (stmt->Bind(baton->parameters)) {
            stmt->status = sqlite3_step(stmt->handle);

            if (!(stmt->status == SQLITE_ROW || stmt->status == SQLITE_DONE)) {
                stmt->message = std::string(sqlite3_errmsg(stmt->db->handle));
            }
        }

        sqlite3_mutex_leave(mtx);

        if (stmt->status == SQLITE_ROW) {
            // Acquire one result row before returning.
            GetRow(&baton->row, stmt->handle);
        }
    }

    return 0;
}

int Statement::EIO_AfterGet(eio_req *req) {
    HandleScope scope;
    STATEMENT_INIT(RowBaton);

    if (stmt->status != SQLITE_ROW && stmt->status != SQLITE_DONE) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            if (stmt->status == SQLITE_ROW) {
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

    STATEMENT_END();
    return 0;
}

Handle<Value> Statement::Run(const Arguments& args) {
    HandleScope scope;
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    Baton* baton = stmt->Bind<Baton>(args);
    stmt->Schedule(EIO_BeginRun, baton);

    return args.This();
}

void Statement::EIO_BeginRun(Baton* baton) {
    STATEMENT_BEGIN(Run);
}

int Statement::EIO_Run(eio_req *req) {
    STATEMENT_INIT(Baton);

    sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->handle);
    sqlite3_mutex_enter(mtx);

    // Make sure that we also reset when there are no parameters.
    if (!baton->parameters.size()) {
        sqlite3_reset(stmt->handle);
    }

    if (stmt->Bind(baton->parameters)) {
        stmt->status = sqlite3_step(stmt->handle);

        if (!(stmt->status == SQLITE_ROW || stmt->status == SQLITE_DONE)) {
            stmt->message = std::string(sqlite3_errmsg(stmt->db->handle));
        }
    }

    sqlite3_mutex_leave(mtx);

    return 0;
}

int Statement::EIO_AfterRun(eio_req *req) {
    HandleScope scope;
    STATEMENT_INIT(Baton);

    if (stmt->status != SQLITE_ROW && stmt->status != SQLITE_DONE) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            Local<Value> argv[] = { Local<Value>::New(Null()) };
            TRY_CATCH_CALL(stmt->handle_, baton->callback, 1, argv);
        }
    }

    STATEMENT_END();
    return 0;
}

Handle<Value> Statement::All(const Arguments& args) {
    HandleScope scope;
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    Baton* baton = stmt->Bind<RowsBaton>(args);
    stmt->Schedule(EIO_BeginAll, baton);

    return args.This();
}

void Statement::EIO_BeginAll(Baton* baton) {
    STATEMENT_BEGIN(All);
}

int Statement::EIO_All(eio_req *req) {
    STATEMENT_INIT(RowsBaton);

    sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->handle);
    sqlite3_mutex_enter(mtx);

    // Make sure that we also reset when there are no parameters.
    if (!baton->parameters.size()) {
        sqlite3_reset(stmt->handle);
    }

    if (stmt->Bind(baton->parameters)) {
        while ((stmt->status = sqlite3_step(stmt->handle)) == SQLITE_ROW) {
            Data::Row* row = new Data::Row();
            GetRow(row, stmt->handle);
            baton->rows.push_back(row);
        }

        if (stmt->status != SQLITE_DONE) {
            stmt->message = std::string(sqlite3_errmsg(stmt->db->handle));
        }
    }

    sqlite3_mutex_leave(mtx);

    return 0;
}

int Statement::EIO_AfterAll(eio_req *req) {
    HandleScope scope;
    STATEMENT_INIT(RowsBaton);

    if (stmt->status != SQLITE_DONE) {
        Error(baton);
    }
    else {
        // Fire callbacks.
        if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
            if (baton->rows.size()) {
                // Create the result array from the data we acquired.
                Local<Array> result(Array::New(baton->rows.size()));
                Data::Rows::const_iterator it = baton->rows.begin();
                Data::Rows::const_iterator end = baton->rows.end();
                for (int i = 0; it < end; it++, i++) {
                    result->Set(i, RowToJS(*it));
                }

                Local<Value> argv[] = { Local<Value>::New(Null()), result };
                TRY_CATCH_CALL(stmt->handle_, baton->callback, 2, argv);
            }
            else {
                // There were no result rows.
                Local<Value> argv[] = {
                    Local<Value>::New(Null()),
                    Local<Value>::New(Array::New(0))
                };
                TRY_CATCH_CALL(stmt->handle_, baton->callback, 2, argv);
            }
        }
    }

    STATEMENT_END();
    return 0;
}

Handle<Value> Statement::Reset(const Arguments& args) {
    HandleScope scope;
    Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

    OPTIONAL_ARGUMENT_FUNCTION(0, callback);

    Baton* baton = new Baton(stmt, callback);
    stmt->Schedule(EIO_BeginReset, baton);

    return args.This();
}

void Statement::EIO_BeginReset(Baton* baton) {
    STATEMENT_BEGIN(Reset);
}

int Statement::EIO_Reset(eio_req *req) {
    STATEMENT_INIT(Baton);

    sqlite3_reset(stmt->handle);
    stmt->status = SQLITE_OK;

    return 0;
}

int Statement::EIO_AfterReset(eio_req *req) {
    HandleScope scope;
    STATEMENT_INIT(Baton);

    // Fire callbacks.
    if (!baton->callback.IsEmpty() && baton->callback->IsFunction()) {
        Local<Value> argv[] = { Local<Value>::New(Null()) };
        TRY_CATCH_CALL(stmt->handle_, baton->callback, 1, argv);
    }

    STATEMENT_END();
    return 0;
}

Local<Array> Statement::RowToJS(Data::Row* row) {
    Local<Array> result(Array::New(row->size()));

    Data::Row::const_iterator it = row->begin();
    Data::Row::const_iterator end = row->end();
    for (int i = 0; it < end; it++, i++) {
        Data::Field* field = *it;
        switch (field->type) {
            case SQLITE_INTEGER: {
                result->Set(i, Local<Integer>(Integer::New(((Data::Integer*)field)->value)));
            } break;
            case SQLITE_FLOAT: {
                result->Set(i, Local<Number>(Number::New(((Data::Float*)field)->value)));
            } break;
            case SQLITE_TEXT: {
                result->Set(i, Local<String>(String::New(((Data::Text*)field)->value.c_str(), ((Data::Text*)field)->value.size())));
            } break;
            // case SQLITE_BLOB: {
            //     result->Set(i, Local<String>(String::New(((Data::Text*)field)->value.c_str())));
            // } break;
            case SQLITE_NULL: {
                result->Set(i, Local<Value>::New(Null()));
            } break;
        }
    }

    return result;
}

void Statement::GetRow(Data::Row* row, sqlite3_stmt* stmt) {
    int rows = sqlite3_column_count(stmt);

    for (int i = 0; i < rows; i++) {
        int type = sqlite3_column_type(stmt, i);
        switch (type) {
            case SQLITE_INTEGER: {
                row->push_back(new Data::Integer(sqlite3_column_int(stmt, i)));
            }   break;
            case SQLITE_FLOAT: {
                row->push_back(new Data::Float(sqlite3_column_double(stmt, i)));
            }   break;
            case SQLITE_TEXT: {
                const char* text = (const char*)sqlite3_column_text(stmt, i);
                int length = sqlite3_column_bytes(stmt, i);
                row->push_back(new Data::Text(length, text));
            } break;
            case SQLITE_BLOB: {
                const void* blob = sqlite3_column_blob(stmt, i);
                int length = sqlite3_column_bytes(stmt, i);
                row->push_back(new Data::Blob(length, blob));
            }   break;
            case SQLITE_NULL: {
                row->push_back(new Data::Null());
            }   break;
            default:
                assert(false);
        }
    }
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
    CleanQueue();
    // Finalize returns the status code of the last operation. We already fired
    // error events in case those failed.
    sqlite3_finalize(handle);
    handle = NULL;
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
