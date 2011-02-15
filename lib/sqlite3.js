// Copyright (c) 2009, Eric Fredricksen <e@fredricksen.net>
// Copyright (c) 2010, Orlando Vazquez <ovazquez@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

var sqlite3 = module.exports = exports = require('./sqlite3_bindings');
var sys = require("sys");

sqlite3.Database.prototype.query = function(sql, bindings, rowCallback) {
  var self = this;

  if (typeof(bindings) == "function") {
    rowCallback = bindings;
    bindings = undefined;
  }

  this.prepare(sql, function(error, statement) {
    function next() {
      _doStep(self, statement, rowCallback);
    }

    if (error) {
      return rowCallback (error);
    }
    if (statement) {
      if (Array.isArray(bindings)) {
        statement.bindArray(bindings, next);
      }
      else if (typeof(bindings) === 'object') {
        statement.bindObject(bindings, next);
      }
      else {
        next();
      }
    }
    else {
      rowCallback();
    }
  });
}

function _doStep(db, statement, rowCallback) {
  statement.step(function (error, row) {
    if (error) {
      return rowCallback(error);
    }

    if (!row) {
      rowCallback();
      statement.finalize(function(){});
      return;
    }
    rowCallback(undefined, row);

    _doStep(db, statement, rowCallback);
  });
}

// Execute a single SQL query with the given optional parameters. Calls
// `callback` with all rows or an error on query completion.
sqlite3.Database.prototype.execute = function (sql /* , bindings, callback */) {
  var self = this;
  var bindings, callback;
  var n = arguments.length;

  switch (n) {
    case 3:
      callback = arguments[2];
      bindings = arguments[1];
      break;
    case 2:
      callback = arguments[1];
      break;
    default: throw new Error("Invalid number of arguments ("+n+")");
  }

  self.prepare(sql, function (error, statement) {
    function next (error) {
      if (error) return callback(new Error("Error binding: " + error.toString()));
      fetchAll(statement);
    }

    if (error) {
      return callback(error);
    }
    if (bindings) {
      if (Array.isArray(bindings)) {
        statement.bindArray(bindings, next);
      }
      else if (typeof(bindings) === 'object') {
        statement.bindObject(bindings, next);
      }
    }
    else {
      next();
    }

    function fetchAll(statement) {
      statement.fetchAll(function (error, rows) {
        if (error) {
          return callback(error);
        }
        statement.finalize(function () {
          callback(undefined, rows);
        });
      });
    }
  });
}

// Execute SQL statements separated by semi-colons.
// SQL must contain no placeholders. Results are discarded.
sqlite3.Database.prototype.executeScript = function (script, callback) {
  var self = this;

  (function stepOverSQL(sql) {
    self.prepare(sql, function(error, statement) {
      if (error) {
        return callback(error);
      }

      statement.step(function (error, row) {
        var tail;
        if (error) {
          callback(error);
          return;
        }
        if (!row) {
          statement.finalize(function(){});

          tail = statement.tail;
          if (typeof tail == "string") {
            tail = tail.trim();
          }
          if (tail) {
            stepOverSQL(tail);
          }
          else {
            callback();
          }
        }
      });
    });
  })(script);
}

sqlite3.Database.prototype.insertMany = function (table, columns, rows, callback) {
  var columnsFragment = columns.join(",");
  var placeholdersFragment = [];
  var i = columns.length;

  if (!rows.length) {
    callback();
    return;
  }

  while (i--) {
    placeholdersFragment.push('?');
  }
  placeholdersFragment = placeholdersFragment.join(", ");

  var sql = [ 'INSERT INTO'
            , table
            , '('
            , columnsFragment
            , ')'
            , 'VALUES'
            , '('
            , placeholdersFragment
            , ')'
            ]
            .join(" ");

  var i = rows.length;

  var statement;

  function doStep(i) {
    statement.bindArray(rows[i], function () {
      statement.step(function (error, row) {
        if (error) return callback(error);
        statement.reset();
        if (i) {
          doStep(--i);
        }
        else {
          statement.finalize(function () {
            callback();
          });
        }
      });
    });
  }

  this.prepare(sql, function (error, stmt) {
    if (error) return callback(error);
    statement = stmt;
    doStep(--i);
  });
}


sqlite3.fromErrorCode = function(code) {
  switch (code) {
    case 0: return "SQLITE_OK";
    case 1: return "SQLITE_ERROR";
    case 2: return "SQLITE_INTERNAL";
    case 3: return "SQLITE_PERM";
    case 4: return "SQLITE_ABORT";
    case 5: return "SQLITE_BUSY";
    case 6: return "SQLITE_LOCKED";
    case 7: return "SQLITE_NOMEM";
    case 8: return "SQLITE_READONLY";
    case 9: return "SQLITE_INTERRUPT";
    case 10: return "SQLITE_IOERR";
    case 11: return "SQLITE_CORRUPT";
    case 12: return "SQLITE_NOTFOUND";
    case 13: return "SQLITE_FULL";
    case 14: return "SQLITE_CANTOPEN";
    case 15: return "SQLITE_PROTOCOL";
    case 16: return "SQLITE_EMPTY";
    case 17: return "SQLITE_SCHEMA";
    case 18: return "SQLITE_TOOBIG";
    case 19: return "SQLITE_CONSTRAINT";
    case 20: return "SQLITE_MISMATCH";
    case 21: return "SQLITE_MISUSE";
    case 22: return "SQLITE_NOLFS";
    case 23: return "SQLITE_AUTH";
    case 24: return "SQLITE_FORMAT";
    case 25: return "SQLITE_RANGE";
    case 26: return "SQLITE_NOTADB";
  }
};

sqlite3.sanitizeError = function(err, data) {
  err.message = exports.fromErrorCode(err.errno) + ', ' + err.message +
                ' in query "' + err.query +
                '" with values ' + JSON.stringify(data, false, 4);
  return err;
};
