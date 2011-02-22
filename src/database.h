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

    static struct Baton {
        Database* db;
        Persistent<Function> callback;
        int status;
        std::string message;

        Baton(Database* db_, Handle<Function> cb_) : db(db_) {
            db->Ref();
            ev_ref(EV_DEFAULT_UC);
            callback = Persistent<Function>::New(cb_);
        }
        ~Baton() {
            db->Unref();
            ev_unref(EV_DEFAULT_UC);
            callback.Dispose();
        }
    };

    static struct OpenBaton : Baton {
        std::string filename;
        int mode;

        OpenBaton(Database* db_, Handle<Function> cb_) : Baton(db_, cb_) {}
    };

    typedef void (*EIO_Callback)(Baton* baton);

    struct Call {
        Call(EIO_Callback cb_, Baton* baton_, bool exclusive_ = false) :
            callback(cb_), exclusive(exclusive_), baton(baton_) {};
        EIO_Callback callback;
        bool exclusive;
        Baton* baton;
    };

    friend class Statement;

protected:
    Database() : EventEmitter(),
        handle(NULL),
        open(false),
        locked(false),
        pending(0),
        serialize(false) {

    }

    ~Database() {
        assert(handle == NULL);
    }

    static Handle<Value> New(const Arguments& args);
    static void EIO_BeginOpen(Baton* baton);
    static int EIO_Open(eio_req *req);
    static int EIO_AfterOpen(eio_req *req);

    void Schedule(EIO_Callback callback, Baton* baton, bool exclusive = false);
    void Process();

    static Handle<Value> Close(const Arguments& args);
    static void EIO_BeginClose(Baton* baton);
    static int EIO_Close(eio_req *req);
    static int EIO_AfterClose(eio_req *req);

    static Handle<Value> Serialize(const Arguments& args);
    static Handle<Value> Parallelize(const Arguments& args);

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
};

}

#endif
