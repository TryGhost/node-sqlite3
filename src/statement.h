#ifndef NODE_SQLITE3_SRC_STATEMENT_H
#define NODE_SQLITE3_SRC_STATEMENT_H

#include <cstdlib>
#include <cstring>
#include <string>
#include <queue>
#include <vector>
#include <sqlite3.h>
#include <napi.h>
#include <uv.h>

#include "database.h"
#include "threading.h"

using namespace Napi;

namespace node_sqlite3 {

namespace Values {
    struct Field {
        inline Field(unsigned short _index, unsigned short _type = SQLITE_NULL) :
            type(_type), index(_index) {}
        inline Field(const char* _name, unsigned short _type = SQLITE_NULL) :
            type(_type), index(0), name(_name) {}

        unsigned short type;
        unsigned short index;
        std::string name;

        virtual ~Field() = default;
    };

    struct Integer : Field {
        template <class T> inline Integer(T _name, int64_t val) :
            Field(_name, SQLITE_INTEGER), value(val) {}
        int64_t value;
        virtual ~Integer() override = default;
    };

    struct Float : Field {
        template <class T> inline Float(T _name, double val) :
            Field(_name, SQLITE_FLOAT), value(val) {}
        double value;
        virtual ~Float() override = default;
    };

    struct Text : Field {
        template <class T> inline Text(T _name, size_t len, const char* val) :
            Field(_name, SQLITE_TEXT), value(val, len) {}
        std::string value;
        virtual ~Text() override = default;
    };

    struct Blob : Field {
        template <class T> inline Blob(T _name, size_t len, const void* val) :
                Field(_name, SQLITE_BLOB), length(len) {
            value = new char[len];
            assert(value != nullptr);
            memcpy(value, val, len);
        }
        inline virtual ~Blob() override {
            delete[] value;
        }
        int length;
        char* value;
    };

    typedef Field Null;
}

typedef std::vector<std::unique_ptr<Values::Field>> Row;
typedef std::deque<std::unique_ptr<Row>> Rows;
typedef std::vector<std::shared_ptr<Values::Field>> Parameters;



class Statement : public Napi::ObjectWrap<Statement> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::Value New(const Napi::CallbackInfo& info);

    struct ParamsBaton {
        Parameters parameters;

        ParamsBaton() {}
        virtual ~ParamsBaton() {}
    };

    struct Baton {
        napi_async_work request = NULL;
        Statement* stmt;
        Napi::Promise::Deferred deferred;
        // TODO: get rid of this (unused)?
        Napi::Reference<Napi::Promise> promise;
        Parameters parameters;
        bool bound = false;

        Baton(Statement* stmt_) :
                stmt(stmt_),
                deferred(Napi::Promise::Deferred::New(stmt_->Env())),
                promise(Napi::Persistent(deferred.Promise())) {
            stmt->Ref();
            promise.Ref();
        }
        virtual ~Baton() {
            parameters.clear();
            if (request) napi_delete_async_work(stmt->Env(), request);
            stmt->Unref();
            promise.Unref();
        }
    };

    class Worker : public Napi::AsyncWorker {
        public:
            Worker(Napi::Env& env, Baton* baton);
            virtual ~Worker(){};

            void Execute();
            void OnOK();
            void OnError(const Napi::Error& e);

        private:
            Baton* baton;
    };


    struct RowBaton : Baton {
        RowBaton(Statement* stmt_) :
            Baton(stmt_) {}
        Row row;
        virtual ~RowBaton() override = default;
    };

    struct RunBaton : Baton {
        RunBaton(Statement* stmt_) :
            Baton(stmt_), inserted_id(0), changes(0) {}
        sqlite3_int64 inserted_id;
        int changes;
        virtual ~RunBaton() override = default;
    };

    struct RowsBaton : Baton {
        RowsBaton(Statement* stmt_) :
            Baton(stmt_) {}
        Rows rows;
        virtual ~RowsBaton() override = default;
    };

    struct Async;

    struct EachBaton : Baton {
        Async* async; // Isn't deleted when the baton is deleted.

        EachBaton(Statement* stmt_) :
            Baton(stmt_) {}
        virtual ~EachBaton() override = default;
    };

    struct PrepareBaton : Database::Baton {
        Statement* stmt;
        std::string sql;
        PrepareBaton(Database* db_, Statement* stmt_) :
            Baton(db_, Napi::Promise::Deferred::New(stmt_->Env())), stmt(stmt_) {
            stmt->Ref();
        }
        virtual ~PrepareBaton() override {
            stmt->Unref();
            if (!db->IsOpen() && db->IsLocked()) {
                // The database handle was closed before the statement could be
                // prepared.
                stmt->Finalize_();
            }
        }
    };

    typedef void (*Work_Callback)(Baton* baton);

    struct Call {
        Call(Work_Callback cb_, Baton* baton_) : callback(cb_), baton(baton_) {};
        Work_Callback callback;
        Baton* baton;
    };

    struct Async {
        uv_async_t watcher;
        Statement* stmt;
        Rows data;
        std::deque<std::shared_ptr<Napi::Promise::Deferred>> deferreds;
        NODE_SQLITE3_MUTEX_t;
        bool completed;
        int retrieved;

        // Store the callbacks here because we don't have
        // access to the baton in the async callback.
        Napi::FunctionReference item_cb;
        Napi::FunctionReference completed_cb;

        Async(Statement* st, uv_async_cb async_cb) :
                stmt(st), completed(false), retrieved(0) {
            watcher.data = this;
            NODE_SQLITE3_MUTEX_INIT
            stmt->Ref();
            uv_loop_t *loop;
            napi_get_uv_event_loop(stmt->Env(), &loop);
            uv_async_init(loop, &watcher, async_cb);
        }

        ~Async() {
            stmt->Unref();
            item_cb.Reset();
            completed_cb.Reset();
            NODE_SQLITE3_MUTEX_DESTROY
        }
    };

    Statement(const Napi::CallbackInfo& info);

    ~Statement() {
        if (!finalized) Finalize_();
    }

    WORK_DEFINITION(Bind)
    WORK_DEFINITION(Get)
    WORK_DEFINITION(Run)
    WORK_DEFINITION(All)
    WORK_DEFINITION(Each)
    WORK_DEFINITION(Reset)

    static void StepEach(Baton* baton);

    Napi::Value Finalize_(const Napi::CallbackInfo& info);

protected:
    Napi::Value Prepare(const Napi::CallbackInfo& info);                          \
    static void Work_BeginPrepare(Database::Baton* baton);
    static void Work_Prepare(napi_env env, void* data);
    static void Work_AfterPrepare(napi_env env, napi_status status, void* data);

    Napi::Value InitEachIterator(EachBaton* baton);
    static void AsyncEach(uv_async_t* handle);
    static void CloseCallback(uv_handle_t* handle);

    static void Finalize_(Baton* baton);
    void Finalize_();

    bool BindParameters(const Napi::CallbackInfo& info, Parameters& parameters);
    template <class T> inline std::shared_ptr<Values::Field> BindParameter(const Napi::Value source, T pos);
    template <class T> T* Bind(const Napi::CallbackInfo& info);
    bool Bind(const Parameters &parameters);

    static void GetRow(Row* row, sqlite3_stmt* stmt);
    static Napi::Value RowToJS(Napi::Env env, Row* row);
    void Schedule(Work_Callback callback, Baton* baton);
    void Process();
    void CleanQueue();
    template <class T> static void Error(T* baton);

protected:
    Database* db;

    std::string sql;

    sqlite3_stmt* _handle = NULL;
    int status = SQLITE_OK;
    bool prepared = false;
    bool locked = true;
    bool finalized = false;

    std::queue<Call*> queue;
    std::string message;
};

}

#endif
