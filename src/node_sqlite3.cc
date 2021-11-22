#include <stdint.h>
#include <sstream>
#include <cstring>
#include <string>
#include <sqlite3.h>

#include "macros.h"
#include "database.h"
#include "statement.h"
#include "backup.h"

using namespace node_sqlite3;

namespace {

Napi::Object RegisterModule(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Database::Init(env, exports);
    Statement::Init(env, exports);
    Backup::Init(env, exports);

    exports.DefineProperties({
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_OPEN_READONLY, OPEN_READONLY)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_OPEN_READWRITE, OPEN_READWRITE)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_OPEN_CREATE, OPEN_CREATE)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_OPEN_FULLMUTEX, OPEN_FULLMUTEX)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_OPEN_URI, OPEN_URI)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_OPEN_SHAREDCACHE, OPEN_SHAREDCACHE)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_OPEN_PRIVATECACHE, OPEN_PRIVATECACHE)
        DEFINE_CONSTANT_STRING(exports, SQLITE_VERSION, VERSION)
#ifdef SQLITE_SOURCE_ID
        DEFINE_CONSTANT_STRING(exports, SQLITE_SOURCE_ID, SOURCE_ID)
#endif
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_VERSION_NUMBER, VERSION_NUMBER)

        DEFINE_CONSTANT_INTEGER(exports, SQLITE_OK, OK)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_ERROR, ERROR)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_INTERNAL, INTERNAL)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_PERM, PERM)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_ABORT, ABORT)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_BUSY, BUSY)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_LOCKED, LOCKED)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_NOMEM, NOMEM)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_READONLY, READONLY)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_INTERRUPT, INTERRUPT)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_IOERR, IOERR)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_CORRUPT, CORRUPT)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_NOTFOUND, NOTFOUND)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_FULL, FULL)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_CANTOPEN, CANTOPEN)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_PROTOCOL, PROTOCOL)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_EMPTY, EMPTY)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_SCHEMA, SCHEMA)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_TOOBIG, TOOBIG)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_CONSTRAINT, CONSTRAINT)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_MISMATCH, MISMATCH)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_MISUSE, MISUSE)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_NOLFS, NOLFS)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_AUTH, AUTH)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_FORMAT, FORMAT)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_RANGE, RANGE)
        DEFINE_CONSTANT_INTEGER(exports, SQLITE_NOTADB, NOTADB)
    });

    return exports;
}

}

const char* sqlite_code_string(int code) {
    switch (code) {
        
        // Basic Codes
        case SQLITE_OK:         return "SQLITE_OK";
        case SQLITE_ERROR:      return "SQLITE_ERROR";
        case SQLITE_INTERNAL:   return "SQLITE_INTERNAL";
        case SQLITE_PERM:       return "SQLITE_PERM";
        case SQLITE_ABORT:      return "SQLITE_ABORT";
        case SQLITE_BUSY:       return "SQLITE_BUSY";
        case SQLITE_LOCKED:     return "SQLITE_LOCKED";
        case SQLITE_NOMEM:      return "SQLITE_NOMEM";
        case SQLITE_READONLY:   return "SQLITE_READONLY";
        case SQLITE_INTERRUPT:  return "SQLITE_INTERRUPT";
        case SQLITE_IOERR:      return "SQLITE_IOERR";
        case SQLITE_CORRUPT:    return "SQLITE_CORRUPT";
        case SQLITE_NOTFOUND:   return "SQLITE_NOTFOUND";
        case SQLITE_FULL:       return "SQLITE_FULL";
        case SQLITE_CANTOPEN:   return "SQLITE_CANTOPEN";
        case SQLITE_PROTOCOL:   return "SQLITE_PROTOCOL";
        case SQLITE_EMPTY:      return "SQLITE_EMPTY";
        case SQLITE_SCHEMA:     return "SQLITE_SCHEMA";
        case SQLITE_TOOBIG:     return "SQLITE_TOOBIG";
        case SQLITE_CONSTRAINT: return "SQLITE_CONSTRAINT";
        case SQLITE_MISMATCH:   return "SQLITE_MISMATCH";
        case SQLITE_MISUSE:     return "SQLITE_MISUSE";
        case SQLITE_NOLFS:      return "SQLITE_NOLFS";
        case SQLITE_AUTH:       return "SQLITE_AUTH";
        case SQLITE_FORMAT:     return "SQLITE_FORMAT";
        case SQLITE_RANGE:      return "SQLITE_RANGE";
        case SQLITE_NOTADB:     return "SQLITE_NOTADB";
        case SQLITE_ROW:        return "SQLITE_ROW";
        case SQLITE_DONE:       return "SQLITE_DONE";

        // Extended Codes
        case SQLITE_ERROR_MISSING_COLLSEQ:      return "SQLITE_ERROR_MISSING_COLLSEQ";
        case SQLITE_ERROR_RETRY:                return "SQLITE_ERROR_RETRY";
        case SQLITE_IOERR_READ:                 return "SQLITE_IOERR_READ";
        case SQLITE_IOERR_SHORT_READ:           return "SQLITE_IOERR_SHORT_READ";
        case SQLITE_IOERR_WRITE:                return "SQLITE_IOERR_WRITE";
        case SQLITE_IOERR_FSYNC:                return "SQLITE_IOERR_FSYNC";
        case SQLITE_IOERR_DIR_FSYNC:            return "SQLITE_IOERR_DIR_FSYNC";
        case SQLITE_IOERR_TRUNCATE:             return "SQLITE_IOERR_TRUNCATE";
        case SQLITE_IOERR_FSTAT:                return "SQLITE_IOERR_FSTAT";
        case SQLITE_IOERR_UNLOCK:               return "SQLITE_IOERR_UNLOCK";
        case SQLITE_IOERR_RDLOCK:               return "SQLITE_IOERR_RDLOCK";
        case SQLITE_IOERR_DELETE:               return "SQLITE_IOERR_DELETE";
        case SQLITE_IOERR_BLOCKED:              return "SQLITE_IOERR_BLOCKED";
        case SQLITE_IOERR_NOMEM:                return "SQLITE_IOERR_NOMEM";
        case SQLITE_IOERR_ACCESS:               return "SQLITE_IOERR_ACCESS";
        case SQLITE_IOERR_CHECKRESERVEDLOCK:    return "SQLITE_IOERR_CHECKRESERVEDLOCK";
        case SQLITE_IOERR_LOCK:                 return "SQLITE_IOERR_LOCK";
        case SQLITE_IOERR_CLOSE:                return "SQLITE_IOERR_CLOSE";
        case SQLITE_IOERR_DIR_CLOSE:            return "SQLITE_IOERR_DIR_CLOSE";
        case SQLITE_IOERR_SHMOPEN:              return "SQLITE_IOERR_SHMOPEN";
        case SQLITE_IOERR_SHMSIZE:              return "SQLITE_IOERR_SHMSIZE";
        case SQLITE_IOERR_SHMLOCK:              return "SQLITE_IOERR_SHMLOCK";
        case SQLITE_IOERR_SHMMAP:               return "SQLITE_IOERR_SHMMAP";
        case SQLITE_IOERR_SEEK:                 return "SQLITE_IOERR_SEEK";
        case SQLITE_IOERR_DELETE_NOENT:         return "SQLITE_IOERR_DELETE_NOENT";
        case SQLITE_IOERR_MMAP:                 return "SQLITE_IOERR_MMAP";
        case SQLITE_IOERR_GETTEMPPATH:          return "SQLITE_IOERR_GETTEMPPATH";
        case SQLITE_IOERR_CONVPATH:             return "SQLITE_IOERR_CONVPATH";
        case SQLITE_IOERR_VNODE:                return "SQLITE_IOERR_VNODE";
        case SQLITE_IOERR_AUTH:                 return "SQLITE_IOERR_AUTH";
        case SQLITE_IOERR_BEGIN_ATOMIC:         return "SQLITE_IOERR_BEGIN_ATOMIC";
        case SQLITE_IOERR_COMMIT_ATOMIC:        return "SQLITE_IOERR_COMMIT_ATOMIC";
        case SQLITE_IOERR_ROLLBACK_ATOMIC:      return "SQLITE_IOERR_ROLLBACK_ATOMIC";
        case SQLITE_LOCKED_SHAREDCACHE:         return "SQLITE_LOCKED_SHAREDCACHE";
        case SQLITE_LOCKED_VTAB:                return "SQLITE_LOCKED_VTAB";
        case SQLITE_BUSY_RECOVERY:              return "SQLITE_BUSY_RECOVERY";
        case SQLITE_BUSY_SNAPSHOT:              return "SQLITE_BUSY_SNAPSHOT";
        case SQLITE_CANTOPEN_NOTEMPDIR:         return "SQLITE_CANTOPEN_NOTEMPDIR";
        case SQLITE_CANTOPEN_ISDIR:             return "SQLITE_CANTOPEN_ISDIR";
        case SQLITE_CANTOPEN_FULLPATH:          return "SQLITE_CANTOPEN_FULLPATH";
        case SQLITE_CANTOPEN_CONVPATH:          return "SQLITE_CANTOPEN_CONVPATH";
        case SQLITE_CORRUPT_VTAB:               return "SQLITE_CORRUPT_VTAB";
        case SQLITE_CORRUPT_SEQUENCE:           return "SQLITE_CORRUPT_SEQUENCE";
        case SQLITE_READONLY_RECOVERY:          return "SQLITE_READONLY_RECOVERY";
        case SQLITE_READONLY_CANTLOCK:          return "SQLITE_READONLY_CANTLOCK";
        case SQLITE_READONLY_ROLLBACK:          return "SQLITE_READONLY_ROLLBACK";
        case SQLITE_READONLY_DBMOVED:           return "SQLITE_READONLY_DBMOVED";
        case SQLITE_READONLY_CANTINIT:          return "SQLITE_READONLY_CANTINIT";
        case SQLITE_READONLY_DIRECTORY:         return "SQLITE_READONLY_DIRECTORY";
        case SQLITE_ABORT_ROLLBACK:             return "SQLITE_ABORT_ROLLBACK";
        case SQLITE_CONSTRAINT_CHECK:           return "SQLITE_CONSTRAINT_CHECK";
        case SQLITE_CONSTRAINT_COMMITHOOK:      return "SQLITE_CONSTRAINT_COMMITHOOK";
        case SQLITE_CONSTRAINT_FOREIGNKEY:      return "SQLITE_CONSTRAINT_FOREIGNKEY";
        case SQLITE_CONSTRAINT_FUNCTION:        return "SQLITE_CONSTRAINT_FUNCTION";
        case SQLITE_CONSTRAINT_NOTNULL:         return "SQLITE_CONSTRAINT_NOTNULL";
        case SQLITE_CONSTRAINT_PRIMARYKEY:      return "SQLITE_CONSTRAINT_PRIMARYKEY";
        case SQLITE_CONSTRAINT_TRIGGER:         return "SQLITE_CONSTRAINT_TRIGGER";
        case SQLITE_CONSTRAINT_UNIQUE:          return "SQLITE_CONSTRAINT_UNIQUE";
        case SQLITE_CONSTRAINT_VTAB:            return "SQLITE_CONSTRAINT_VTAB";
        case SQLITE_CONSTRAINT_ROWID:           return "SQLITE_CONSTRAINT_ROWID";
        case SQLITE_NOTICE_RECOVER_WAL:         return "SQLITE_NOTICE_RECOVER_WAL";
        case SQLITE_NOTICE_RECOVER_ROLLBACK:    return "SQLITE_NOTICE_RECOVER_ROLLBACK";
        case SQLITE_WARNING_AUTOINDEX:          return "SQLITE_WARNING_AUTOINDEX";
        case SQLITE_AUTH_USER:                  return "SQLITE_AUTH_USER";
        case SQLITE_OK_LOAD_PERMANENTLY:        return "SQLITE_OK_LOAD_PERMANENTLY";

        default: return "UNKNOWN";
    }
}

const char* sqlite_authorizer_string(int type) {
    switch (type) {
        case SQLITE_INSERT:     return "insert";
        case SQLITE_UPDATE:     return "update";
        case SQLITE_DELETE:     return "delete";
        default:                return "";
    }
}

NODE_API_MODULE(node_sqlite3, RegisterModule)
