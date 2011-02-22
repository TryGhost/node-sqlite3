#ifndef NODE_SQLITE3_SRC_STATEMENT_H
#define NODE_SQLITE3_SRC_STATEMENT_H

#include <v8.h>
#include <node.h>
#include <node_events.h>

#include "database.h"

#include <typeinfo>
#include <string>
#include <queue>
#include <vector>

#include <sqlite3.h>

using namespace v8;
using namespace node;

namespace node_sqlite3 {

namespace Data {
    struct Field {
        inline Field(unsigned short _type = SQLITE_NULL) : type(_type) {}
        unsigned short type;
    };

    struct Integer : Field {
        inline Integer(int val) : Field(SQLITE_INTEGER), value(val) {}
        int value;
    };

    struct Float : Field {
        inline Float(double val) : Field(SQLITE_FLOAT), value(val) {}
        double value;
    };

    struct Text : Field {
        inline Text(size_t len, const char* val) : Field(SQLITE_TEXT), value(val, len) {}
        std::string value;
    };

    struct Blob : Field {
        inline Blob(size_t len, const void* val) : Field(SQLITE_BLOB), length(len) {
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
    typedef std::vector<Field*> Row;
    typedef std::vector<Row*> Rows;


    struct Parameter {
        unsigned short position;
        std::string name;
        Field* field;

        inline Parameter(unsigned short pos_, Field* field_) : position(pos_), field(field_) {}
        inline Parameter(const char* name_, Field* field_) : position(0), name(name_), field(field_) {}
        inline ~Parameter() {
            if (field) {
                delete field;
            }
        }
    };

    typedef std::vector<Parameter*> Parameters;
}




class Statement : public EventEmitter {
public:
    static Persistent<FunctionTemplate> constructor_template;

    static void Init(Handle<Object> target);
    static Handle<Value> New(const Arguments& args);

    static struct Baton {
        Statement* stmt;
        Persistent<Function> callback;
        Data::Parameters parameters;

        Baton(Statement* stmt_, Handle<Function> cb_) : stmt(stmt_) {
            Data::Parameters::const_iterator it = parameters.begin();
            Data::Parameters::const_iterator end = parameters.end();
            for (; it < end; it++) delete *it;

            stmt->Ref();
            ev_ref(EV_DEFAULT_UC);
            callback = Persistent<Function>::New(cb_);
        }
        ~Baton() {
            stmt->Unref();
            ev_unref(EV_DEFAULT_UC);
            callback.Dispose();
        }
    };

    static struct RowBaton : Baton {
        RowBaton(Statement* stmt_, Handle<Function> cb_) :
            Baton(stmt_, cb_) {}
        Data::Row row;
    };

    static struct RowsBaton : Baton {
        RowsBaton(Statement* stmt_, Handle<Function> cb_) :
            Baton(stmt_, cb_) {}
        Data::Rows rows;
    };

    static struct PrepareBaton : Database::Baton {
        Statement* stmt;
        std::string sql;
        PrepareBaton(Database* db_, Handle<Function> cb_, Statement* stmt_) :
            Baton(db_, cb_), stmt(stmt_) {
            stmt->Ref();
        }
        ~PrepareBaton() {
            stmt->Unref();
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
        Data::Rows data;
        pthread_mutex_t mutex;
        Persistent<Function> callback;
        bool completed;

        Async(Statement* st, Handle<Function> cb, Async_Callback async_cb) :
                stmt(st), completed(false) {
            watcher.data = this;
            ev_async_init(&watcher, async_cb);
            ev_async_start(EV_DEFAULT_UC_ &watcher);
            callback = Persistent<Function>::New(cb);
            stmt->Ref();
            pthread_mutex_init(&mutex, NULL);
        }

        ~Async() {
            callback.Dispose();
            stmt->Unref();
            pthread_mutex_destroy(&mutex);
            ev_async_stop(EV_DEFAULT_UC_ &watcher);
        }
    };

    Statement(Database* db_) : EventEmitter(),
            db(db_),
            handle(NULL),
            status(SQLITE_OK),
            prepared(false),
            locked(false),
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

    Data::Field* BindParameter(const Handle<Value> source);
    template <class T> T* Bind(const Arguments& args, int start = 0);
    bool Bind(const Data::Parameters parameters);

    static void GetRow(Data::Row* row, sqlite3_stmt* stmt);
    static Local<Array> RowToJS(Data::Row* row);
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
