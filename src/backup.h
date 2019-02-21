#ifndef NODE_SQLITE3_SRC_BACKUP_H
#define NODE_SQLITE3_SRC_BACKUP_H

#include "database.h"

#include <string>
#include <queue>
#include <set>

#include <sqlite3.h>
#include <nan.h>

using namespace v8;
using namespace node;

namespace node_sqlite3 {

/**
 *
 * A class for managing an sqlite3_backup object.  For consistency
 * with other node-sqlite3 classes, it maintains an internal queue
 * of calls.
 *
 * Intended usage from node:
 *
 *   var db = new sqlite3.Database('live.db');
 *   var backup = db.backup('backup.db');
 *   ...
 *   // in event loop, move backup forward when we have time.
 *   if (backup.idle) { backup.step(NPAGES); }
 *   if (backup.completed) { ... success ... }
 *   if (backup.failed)    { ... sadness ... }
 *   // do other work in event loop - fine to modify live.db
 *   ...
 *
 * Here is how sqlite's backup api is exposed:
 *
 *   - `sqlite3_backup_init`: This is implemented as
 *     `db.backup(filename, [callback])` or
 *     `db.backup(filename, destDbName, sourceDbName, filenameIsDest, [callback])`.
 *   - `sqlite3_backup_step`: `backup.step(pages, [callback])`.
 *   - `sqlite3_backup_finish`: `backup.finish([callback])`.
 *   - `sqlite3_backup_remaining`: `backup.remaining`.
 *   - `sqlite3_backup_pagecount`: `backup.pageCount`.
 *
 * There are the following read-only properties:
 *
 *   - `backup.completed` is set to `true` when the backup
 *     succeeeds.
 *   - `backup.failed` is set to `true` when the backup
 *     has a fatal error.
 *   - `backup.idle` is set to `true` when no operation
 *     is currently in progress or queued for the backup.
 *   - `backup.remaining` is an integer with the remaining
 *     number of pages after the last call to `backup.step`
 *     (-1 if `step` not yet called).
 *   - `backup.pageCount` is an integer with the total number
 *     of pages measured during the last call to `backup.step`
 *     (-1 if `step` not yet called).
 *
 * There is the following writable property:
 *
 *   - `backup.retryErrors`: an array of sqlite3 error codes
 *     that are treated as non-fatal - meaning, if they occur,
 *     backup.failed is not set, and the backup may continue.
 *     By default, this is `[sqlite3.BUSY, sqlite3.LOCKED]`.
 *
 * The `db.backup(filename, [callback])` shorthand is sufficient
 * for making a backup of a database opened by node-sqlite3.  If
 * using attached or temporary databases, or moving data in the
 * opposite direction, the more complete (but daunting)
 * `db.backup(filename, destDbName, sourceDbName, filenameIsDest, [callback])`
 * signature is provided.
 *
 * A backup will finish automatically when it succeeds or a fatal
 * error occurs, meaning it is not necessary to call `db.finish()`.
 * By default, SQLITE_LOCKED and SQLITE_BUSY errors are not
 * treated as failures, and the backup will continue if they
 * occur.  The set of errors that are tolerated can be controlled
 * by setting `backup.retryErrors`. To disable automatic
 * finishing and stick strictly to sqlite's raw api, set
 * `backup.retryErrors` to `[]`.  In that case, it is necessary
 * to call `backup.finish()`.
 *
 * In the same way as node-sqlite3 databases and statements,
 * backup methods can be called safely without callbacks, due
 * to an internal call queue.  So for example this naive code
 * will correctly back up a db, if there are no errors:
 *
 *   var backup = db.backup('backup.db');
 *   backup.step(-1);
 *   backup.finish();
 *
 */
class Backup : public Nan::ObjectWrap {
public:
    static Nan::Persistent<FunctionTemplate> constructor_template;

    static NAN_MODULE_INIT(Init);
    static NAN_METHOD(New);

    struct Baton {
        uv_work_t request;
        Backup* backup;
        Nan::Persistent<Function> callback;

        Baton(Backup* backup_, Local<Function> cb_) : backup(backup_) {
            backup->Ref();
            request.data = this;
            callback.Reset(cb_);
        }
        virtual ~Baton() {
            backup->Unref();
            callback.Reset();
        }
    };

    struct InitializeBaton : Database::Baton {
        Backup* backup;
        std::string filename;
        std::string sourceName;
        std::string destName;
        bool filenameIsDest;
        InitializeBaton(Database* db_, Local<Function> cb_, Backup* backup_) :
            Baton(db_, cb_), backup(backup_), filenameIsDest(true) {
            backup->Ref();
        }
        virtual ~InitializeBaton() {
            backup->Unref();
            if (!db->IsOpen() && db->IsLocked()) {
                // The database handle was closed before the backup could be opened.
                backup->FinishAll();
            }
        }
    };

    struct StepBaton : Baton {
        int pages;
        std::set<int> retryErrorsSet;
        StepBaton(Backup* backup_, Local<Function> cb_, int pages_) :
            Baton(backup_, cb_), pages(pages_) {}
    };

    typedef void (*Work_Callback)(Baton* baton);

    struct Call {
        Call(Work_Callback cb_, Baton* baton_) : callback(cb_), baton(baton_) {};
        Work_Callback callback;
        Baton* baton;
    };

    Backup(Database* db_) : Nan::ObjectWrap(),
           db(db_),
           _handle(NULL),
           _otherDb(NULL),
           _destDb(NULL),
           inited(false),
           locked(true),
           completed(false),
           failed(false),
           remaining(-1),
           pageCount(-1),
           finished(false) {
        db->Ref();
    }

    ~Backup() {
        if (!finished) {
            FinishAll();
        }
        retryErrors.Reset();
    }

    WORK_DEFINITION(Step);
    WORK_DEFINITION(Finish);
    static NAN_GETTER(IdleGetter);
    static NAN_GETTER(CompletedGetter);
    static NAN_GETTER(FailedGetter);
    static NAN_GETTER(PageCountGetter);
    static NAN_GETTER(RemainingGetter);
    static NAN_GETTER(FatalErrorGetter);
    static NAN_GETTER(RetryErrorGetter);

    static NAN_SETTER(FatalErrorSetter);
    static NAN_SETTER(RetryErrorSetter);

protected:
    static void Work_BeginInitialize(Database::Baton* baton);
    static void Work_Initialize(uv_work_t* req);
    static void Work_AfterInitialize(uv_work_t* req);

    void Schedule(Work_Callback callback, Baton* baton);
    void Process();
    void CleanQueue();
    template <class T> static void Error(T* baton);

    void FinishAll();
    void FinishSqlite();
    void GetRetryErrors(std::set<int>& retryErrorsSet);

    Database* db;

    sqlite3_backup* _handle;
    sqlite3* _otherDb;
    sqlite3* _destDb;
    int status;
    std::string message;

    bool inited;
    bool locked;
    bool completed;
    bool failed;
    int remaining;
    int pageCount;
    bool finished;
    std::queue<Call*> queue;

    Nan::Persistent<Array> retryErrors;
};

}

#endif
