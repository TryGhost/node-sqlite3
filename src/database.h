
#ifndef NODE_SQLITE3_SRC_DATABASE_H
#define NODE_SQLITE3_SRC_DATABASE_H

#include <node.h>

#include <string>
#include <queue>

#include <sqlite3.h>
#include "nan.h"
#include "async.h"

using namespace v8;
using namespace node;

namespace node_sqlite3 {

class Database;


class Database : public ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor_template;
    static void Init(Handle<Object> target);

    static inline bool HasInstance(Handle<Value> val) {
        NanScope();
        if (!val->IsObject()) return false;
        Local<Object> obj = val->ToObject();
        return NanNew(constructor_template)->HasInstance(obj);
    }

    struct Baton {
        uv_work_t request;
        Database* db;
        Persistent<Function> callback;
        int status;
        std::string message;

        Baton(Database* db_, Handle<Function> cb_) :
                db(db_), status(SQLITE_OK) {
            db->Ref();
            request.data = this;
            NanAssignPersistent(callback, cb_);
        }
        virtual ~Baton() {
            db->Unref();
            NanDisposePersistent(callback);
        }
    };

    struct OpenBaton : Baton {
        std::string filename;
        int mode;
        OpenBaton(Database* db_, Handle<Function> cb_, const char* filename_, int mode_) :
            Baton(db_, cb_), filename(filename_), mode(mode_) {}
    };

    struct ExecBaton : Baton {
        std::string sql;
        ExecBaton(Database* db_, Handle<Function> cb_, const char* sql_) :
            Baton(db_, cb_), sql(sql_) {}
    };

    struct LoadExtensionBaton : Baton {
        std::string filename;
        LoadExtensionBaton(Database* db_, Handle<Function> cb_, const char* filename_) :
            Baton(db_, cb_), filename(filename_) {}
    };

    struct FunctionInvocation {
        sqlite3_context *context;
        sqlite3_value **argv;
        int argc;
        bool complete;
    };

    struct FunctionBaton {
        Database* db;
        std::string name;
        Persistent<Function> callback;
        uv_async_t async;
        uv_mutex_t mutex;
        uv_cond_t condition;
        std::queue<FunctionInvocation*> queue;

        FunctionBaton(Database* db_, const char* name_, Handle<Function> cb_) :
                db(db_), name(name_) {
            async.data = this;
            NanAssignPersistent(callback, cb_);
        }
        virtual ~FunctionBaton() {
            NanDisposePersistent(callback);
        }
    };

    typedef void (*Work_Callback)(Baton* baton);

    struct Call {
        Call(Work_Callback cb_, Baton* baton_, bool exclusive_ = false) :
            callback(cb_), exclusive(exclusive_), baton(baton_) {};
        Work_Callback callback;
        bool exclusive;
        Baton* baton;
    };

    struct ProfileInfo {
        std::string sql;
        sqlite3_int64 nsecs;
    };

    struct UpdateInfo {
        int type;
        std::string database;
        std::string table;
        sqlite3_int64 rowid;
    };

    bool IsOpen() { return open; }
    bool IsLocked() { return locked; }

    typedef Async<std::string, Database> AsyncTrace;
    typedef Async<ProfileInfo, Database> AsyncProfile;
    typedef Async<UpdateInfo, Database> AsyncUpdate;

    friend class Statement;

protected:
    Database() : ObjectWrap(),
        _handle(NULL),
        open(false),
        locked(false),
        pending(0),
        serialize(false),
        debug_trace(NULL),
        debug_profile(NULL),
        update_event(NULL) {
    }

    ~Database() {
        RemoveCallbacks();
        sqlite3_close(_handle);
        _handle = NULL;
        open = false;
    }

    static NAN_METHOD(New);
    static void Work_BeginOpen(Baton* baton);
    static void Work_Open(uv_work_t* req);
    static void Work_AfterOpen(uv_work_t* req);

    static NAN_GETTER(OpenGetter);

    void Schedule(Work_Callback callback, Baton* baton, bool exclusive = false);
    void Process();

    static NAN_METHOD(Exec);
    static void Work_BeginExec(Baton* baton);
    static void Work_Exec(uv_work_t* req);
    static void Work_AfterExec(uv_work_t* req);

    static NAN_METHOD(Wait);
    static void Work_Wait(Baton* baton);

    static NAN_METHOD(Close);
    static void Work_BeginClose(Baton* baton);
    static void Work_Close(uv_work_t* req);
    static void Work_AfterClose(uv_work_t* req);

    static NAN_METHOD(LoadExtension);
    static void Work_BeginLoadExtension(Baton* baton);
    static void Work_LoadExtension(uv_work_t* req);
    static void Work_AfterLoadExtension(uv_work_t* req);

    static NAN_METHOD(Serialize);
    static NAN_METHOD(Parallelize);

    static NAN_METHOD(Configure);

    static NAN_METHOD(RegisterFunction);
    static void FunctionEnqueue(sqlite3_context *context, int argc, sqlite3_value **argv);
    static void FunctionExecute(FunctionBaton *baton, FunctionInvocation *invocation);
    static void AsyncFunctionProcessQueue(uv_async_t *async);

    static void SetBusyTimeout(Baton* baton);

    static void RegisterTraceCallback(Baton* baton);
    static void TraceCallback(void* db, const char* sql);
    static void TraceCallback(Database* db, std::string* sql);

    static void RegisterProfileCallback(Baton* baton);
    static void ProfileCallback(void* db, const char* sql, sqlite3_uint64 nsecs);
    static void ProfileCallback(Database* db, ProfileInfo* info);

    static void RegisterUpdateCallback(Baton* baton);
    static void UpdateCallback(void* db, int type, const char* database, const char* table, sqlite3_int64 rowid);
    static void UpdateCallback(Database* db, UpdateInfo* info);

    void RemoveCallbacks();

protected:
    sqlite3* _handle;

    bool open;
    bool locked;
    unsigned int pending;

    bool serialize;

    std::queue<Call*> queue;

    AsyncTrace* debug_trace;
    AsyncProfile* debug_profile;
    AsyncUpdate* update_event;
};

}

#endif
