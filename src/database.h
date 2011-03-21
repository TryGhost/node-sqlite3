#ifndef NODE_SQLITE3_SRC_DATABASE_H
#define NODE_SQLITE3_SRC_DATABASE_H

#include <v8.h>
#include <node.h>
#include <node_events.h>

#include <string>
#include <queue>

#include <sqlite3.h>
#include "async.h"

using namespace v8;
using namespace node;

namespace node_sqlite3 {

class Database;


class Database : public EventEmitter {
public:
    static Persistent<FunctionTemplate> constructor_template;
    static void Init(Handle<Object> target);

    static inline bool HasInstance(Handle<Value> val) {
        if (!val->IsObject()) return false;
        Local<Object> obj = val->ToObject();
        return constructor_template->HasInstance(obj);
    }

    struct Baton {
        Database* db;
        Persistent<Function> callback;
        int status;
        std::string message;

        Baton(Database* db_, Handle<Function> cb_) :
                db(db_), status(SQLITE_OK) {
            db->Ref();
            ev_ref(EV_DEFAULT_UC);
            callback = Persistent<Function>::New(cb_);
        }
        virtual ~Baton() {
            db->Unref();
            ev_unref(EV_DEFAULT_UC);
            callback.Dispose();
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

    typedef void (*EIO_Callback)(Baton* baton);

    struct Call {
        Call(EIO_Callback cb_, Baton* baton_, bool exclusive_ = false) :
            callback(cb_), exclusive(exclusive_), baton(baton_) {};
        EIO_Callback callback;
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
        std::string tablename;
        sqlite3_int64 rowid;
    };

    typedef Async<std::string, Database> AsyncTrace;
    typedef Async<ProfileInfo, Database> AsyncProfile;
    typedef Async<UpdateInfo, Database> AsyncUpdate;

    friend class Statement;

protected:
    Database() : EventEmitter(),
        handle(NULL),
        open(false),
        locked(false),
        pending(0),
        serialize(false),
        debug_trace(NULL),
        debug_profile(NULL) {

    }

    ~Database() {
        assert(handle == NULL);
        if (debug_trace) {
            delete debug_trace;
            debug_trace = NULL;
        }
    }

    static Handle<Value> New(const Arguments& args);
    static void EIO_BeginOpen(Baton* baton);
    static int EIO_Open(eio_req *req);
    static int EIO_AfterOpen(eio_req *req);

    static Handle<Value> OpenGetter(Local<String> str, const AccessorInfo& accessor);

    void Schedule(EIO_Callback callback, Baton* baton, bool exclusive = false);
    void Process();

    static Handle<Value> Exec(const Arguments& args);
    static void EIO_BeginExec(Baton* baton);
    static int EIO_Exec(eio_req *req);
    static int EIO_AfterExec(eio_req *req);

    static Handle<Value> Close(const Arguments& args);
    static void EIO_BeginClose(Baton* baton);
    static int EIO_Close(eio_req *req);
    static int EIO_AfterClose(eio_req *req);

    static Handle<Value> LoadExtension(const Arguments& args);
    static void EIO_BeginLoadExtension(Baton* baton);
    static int EIO_LoadExtension(eio_req *req);
    static int EIO_AfterLoadExtension(eio_req *req);

    static Handle<Value> Serialize(const Arguments& args);
    static Handle<Value> Parallelize(const Arguments& args);

    static Handle<Value> Configure(const Arguments& args);

    static void RegisterTraceCallback(Baton* baton);
    static void TraceCallback(void* db, const char* sql);
    static void TraceCallback(Database* db, std::string* sql);

    static void RegisterProfileCallback(Baton* baton);
    static void ProfileCallback(void* db, const char* sql, sqlite3_uint64 nsecs);
    static void ProfileCallback(Database* db, ProfileInfo* info);

    static void RegisterUpdateCallback(Baton* baton);
    static void UpdateCallback(void* db, int type, const char* database, const char* table, sqlite3_uint64 rowid);
    static void UpdateCallback(Database* db, UpdateInfo* info);

    void RemoveCallbacks();
    void Wrap (Handle<Object> handle);
    inline void MakeWeak();
    virtual void Unref();
    static void Destruct (Persistent<Value> value, void *data);
    static int EIO_Destruct(eio_req *req);
    static int EIO_AfterDestruct(eio_req *req);

protected:
    sqlite3* handle;

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
