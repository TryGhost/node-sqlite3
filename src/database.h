#ifndef NODE_SQLITE3_SRC_DATABASE_H
#define NODE_SQLITE3_SRC_DATABASE_H

#include <v8.h>
#include <node.h>
#include <node_events.h>

#include <string>
#include <queue>

#include <sqlite3.h>

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

    typedef void (*Async_Callback)(EV_P_ ev_async *w, int revents);


    // Generic ev_async handler.
    template <class Item, class Parent> class Async {
    protected:
        ev_async watcher;
        pthread_mutex_t mutex;
        std::vector<Item> data;
    public:
        Parent* parent;

    public:
        Async(Parent* parent_, Async_Callback async_cb) : parent(parent_) {
            watcher.data = this;
            ev_async_init(&watcher, async_cb);
            ev_async_start(EV_DEFAULT_UC_ &watcher);
            pthread_mutex_init(&mutex, NULL);
        }

        inline void add(Item item) {
            // Make sure node runs long enough to deliver the messages.
            ev_ref(EV_DEFAULT_UC);
            pthread_mutex_lock(&mutex);
            data.push_back(item);
            pthread_mutex_unlock(&mutex);
        }

        inline std::vector<Item> get() {
            std::vector<Item> rows;
            pthread_mutex_lock(&mutex);
            rows.swap(data);
            pthread_mutex_unlock(&mutex);
            for (int i = rows.size(); i > 0; i--) {
                ev_unref(EV_DEFAULT_UC);
            }
            return rows;
        }

        inline void send() {
            ev_async_send(EV_DEFAULT_ &watcher);
        }

        inline void send(Item item) {
            add(item);
            send();
        }

        ~Async() {
            ev_invoke(&watcher, ev_async_pending(&watcher));
            pthread_mutex_destroy(&mutex);
            ev_async_stop(EV_DEFAULT_UC_ &watcher);
        }
    };

    typedef Async<std::string, Database> AsyncTrace;
    typedef std::pair<std::string, sqlite3_uint64> ProfileInfo;
    typedef Async<ProfileInfo, Database> AsyncProfile;

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
    static void TraceCallback(EV_P_ ev_async *w, int revents);
    static void RegisterProfileCallback(Baton* baton);
    static void ProfileCallback(void* db, const char* sql, sqlite3_uint64 nsecs);
    static void ProfileCallback(EV_P_ ev_async *w, int revents);

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
};

}

#endif
