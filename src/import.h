
#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif

int sqlite_import(sqlite3 *db, const char *zFile, const char *zTable);

#ifdef __cplusplus
}
#endif
