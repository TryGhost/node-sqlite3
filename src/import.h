
#ifndef NODE_SQLITE3_SRC_IMPORT_H
#define NODE_SQLITE3_SRC_IMPORT_H

#include <sqlite3.h>
#include <vector>
#include <string>

struct ImportOptions {
  std::vector<std::string> columnIds;
  char columnDelimiter;

  ImportOptions(std::vector<std::string> const &columnIds_,const char columnDelimiter_) :
  columnIds(columnIds_), columnDelimiter(columnDelimiter_) { }
};

struct ImportResult {
  std::string tableName;
  std::vector<std::string> columnIds;
  std::vector<std::string> columnTypes;
  unsigned int rowCount;

  ImportResult(std::string const &tableName_, std::vector<std::string> const &columnIds_,
    std::vector<std::string> const &columnTypes_, unsigned int rowCount_) :
    tableName(tableName_), columnIds(columnIds_), columnTypes(columnTypes_), rowCount(rowCount_) { }
};

ImportResult *sqlite_import(
  sqlite3 *db,
  const char *zFile,      // CSV file to import
  const char *zTable,     // sqlite destination table name
  ImportOptions &options,  // import options
  std::string &errMsg      // Set in case of error
);

#endif
