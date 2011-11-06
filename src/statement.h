#ifndef NODE_SQLITE3_SRC_STATEMENT_H
#define NODE_SQLITE3_SRC_STATEMENT_H

#include <v8.h>
#include <node.h>
#include <node_events.h>

#include "database.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <queue>
#include <vector>

#include <sqlite3.h>

using namespace v8;
using namespace node;

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
    };

    struct Integer : Field {
        template <class T> inline Integer(T _name, int64_t val) :
            Field(_name, SQLITE_INTEGER), value(val) {}
        int64_t value;
    };

    struct Float : Field {
        template <class T> inline Float(T _name, double val) :
            Field(_name, SQLITE_FLOAT), value(val) {}
        double value;
    };

    struct Text : Field {
        template <class T> inline Text(T _name, size_t len, const char* val) :
            Field(_name, SQLITE_TEXT), value(val, len) {}
        std::string value;
    };

    struct Blob : Field {
        template <class T> inline Blob(T _name, size_t len, const void* val) :
                Field(_name, SQLITE_BLOB), length(len) {
            value = (char*)malloc(len);
            memcpy(value, val, len);
        }
        inline ~Blob() {
            free(value);
        }
        int length;
        char* value;
    };

    typedef Field Null;
}

typedef std::vector<Values::Field*> Row;
typedef std::vector<Row*> Rows;
typedef Row Parameters;



class Statement : public EventEmitter {
public:
    static Persistent<FunctionTemplate> constructor_template;

    static void Init(Handle<Object> target);
    static Handle<Value> New(const Arguments& args);

    struct Baton {
        Statement* stmt;
        Persistent<Function> callback;
        Parameters parameters;

        Baton(Statement* stmt_, Handle<Function> cb_) : stmt(stmt_) {
            stmt->Ref();
            ev_ref(EV_DEFAULT_UC);
            callback = Persistent<Function>::New(cb_);
        }
        ~Baton() {
            for (unsigned int i = 0; i < parameters.size(); i++) {
                Values::Field* field = parameters[i];
                DELETE_FIELD(field);
            }
            stmt->Unref();
            ev_unref(EV_DEFAULT_UC);
            callback.Dispose();
        }
    };

    struct RowBaton : Baton {
        RowBaton(Statement* stmt_, Handle<Function> cb_) :
            Baton(stmt_, cb_) {}
        Row row;
    };

    struct RunBaton : Baton {
        RunBaton(Statement* stmt_, Handle<Function> cb_) :
            Baton(stmt_, cb_), inserted_id(0), changes(0) {}
        sqlite3_int64 inserted_id;
        int changes;
    };

    struct RowsBaton : Baton {
        RowsBaton(Statement* stmt_, Handle<Function> cb_) :
            Baton(stmt_, cb_) {}
        Rows rows;
    };

    struct EachBaton : Baton {
        EachBaton(Statement* stmt_, Handle<Function> cb_) :
            Baton(stmt_, cb_) {}
        Persistent<Function> completed;
    };

    struct PrepareBaton : Database::Baton {
        Statement* stmt;
        std::string sql;
        PrepareBaton(Database* db_, Handle<Function> cb_, Statement* stmt_) :
            Baton(db_, cb_), stmt(stmt_) {
            stmt->Ref();
        }
        virtual ~PrepareBaton() {
            stmt->Unref();
            if (!db->IsOpen() && db->IsLocked()) {
                // The database handle was closed before the statement could be
                // prepared.
                stmt->Finalize();
            }
        }
    };

    typedef void (*EIO_Callback)(Baton* baton);

    struct Call {
        Call(EIO_Callback cb_, Baton* baton_) : callback(cb_), baton(baton_) {};
        EIO_Callback callback;
        Baton* baton;
    };

    typedef void (*Async_Callback)(EV_P_ ev_async *w, int revents);

    struct Async {
        ev_async watcher;
        Statement* stmt;
        Rows data;
        NODE_SQLITE3_MUTEX_t;
        Persistent<Function> callback;
        bool completed;
        int retrieved;
        Persistent<Function> completed_callback;

        Async(Statement* st, Handle<Function> cb, Handle<Function>completed_cb,
              Async_Callback async_cb) :
                stmt(st), completed(false), retrieved(0) {
            watcher.data = this;
            ev_async_init(&watcher, async_cb);
            ev_async_start(EV_DEFAULT_UC_ &watcher);
            callback = Persistent<Function>::New(cb);
            completed_callback = Persistent<Function>::New(completed_cb);
            stmt->Ref();
            NODE_SQLITE3_MUTEX_INIT
        }

        ~Async() {
            callback.Dispose();
            completed_callback.Dispose();
            stmt->Unref();
            NODE_SQLITE3_MUTEX_DESTROY
            ev_async_stop(EV_DEFAULT_UC_ &watcher);
        }
    };

    Statement(Database* db_) : EventEmitter(),
            db(db_),
            handle(NULL),
            status(SQLITE_OK),
            prepared(false),
            locked(true),
            finalized(false) {
        db->Ref();
    }

    ~Statement() {
        if (!finalized) Finalize();
    }

protected:
    static void EIO_BeginPrepare(Database::Baton* baton);
    static int EIO_Prepare(eio_req *req);
    static int EIO_AfterPrepare(eio_req *req);

    EIO_DEFINITION(Bind);
    EIO_DEFINITION(Get);
    EIO_DEFINITION(Run);
    EIO_DEFINITION(All);
    EIO_DEFINITION(Each);
    EIO_DEFINITION(Reset);

    static void AsyncEach(EV_P_ ev_async *w, int revents);

    static Handle<Value> Finalize(const Arguments& args);
    static void Finalize(Baton* baton);
    void Finalize();

    template <class T> inline Values::Field* BindParameter(const Handle<Value> source, T pos);
    template <class T> T* Bind(const Arguments& args, int start = 0, int end = -1);
    bool Bind(const Parameters parameters);

    static void GetRow(Row* row, sqlite3_stmt* stmt);
    static Local<Object> RowToJS(Row* row);
    void Schedule(EIO_Callback callback, Baton* baton);
    void Process();
    void CleanQueue();
    template <class T> static void Error(T* baton);

protected:
    Database* db;

    sqlite3_stmt* handle;
    int status;
    std::string message;

    bool prepared;
    bool locked;
    bool finalized;
    std::queue<Call*> queue;
};

}

#endif
