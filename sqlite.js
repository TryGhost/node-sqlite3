/*
Copyright (c) 2009, Eric Fredricksen <e@fredricksen.net>
Copyright (c) 2010, Orlando Vazquez <ovazquez@gmail.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

var sys = require("sys");
var puts = sys.puts;
var sqlite = require("./sqlite3_bindings");

var Database = exports.Database = sqlite.Database;

Database.prototype.query = function (sql, bindings, queryCallback) {
  var self = this;

  if (typeof(bindings) == "function") {
    var tmp = bindings;
    bindings = callback;
    callback = tmp;
  }

  var all = [];

  // Iterate over the list of bindings. Since we can't use something as
  // simple as a for or while loop, we'll use the event loop
  function doBindingsByIndex(statement, bindings, queryCallback, startIndex) {
    (function (statement, bindings, startIndex) {
      var innerFunction = arguments.callee;
      if (!bindings.length) {
        process.nextTick(function () {
          queryCallback(statement);
        });
        return;
      }

      startIndex = startIndex || 1;
      var value = bindings.shift();

      puts("setting index " + startIndex + " to " + value);
      process.nextTick(function () {
        statement.bind(startIndex, value, function () {
          innerFunction(statement, bindings, startIndex+1);
        });
      });
    })(statement, bindings, startIndex);
  }

  function queryDone(statement, rows) {
    if (statement.tail) {
      puts("omg it has a tail");
      self.prepare(statement.tail, onPrepare);
    }
    queryCallback(undefined, rows);
  }

  function doStep(statement) {
    var rows = [];
    (function () {
      var innerFunction = arguments.callee;
      statement.step(function (error, row) {
        if (error) throw error;
        if (!row) {
//           rows.rowsAffected = this.changes();
//           rows.insertId = this.lastInsertRowid();
          process.nextTick(function () {
            queryDone(statement, rows);
          });
          return;
        }
        rows.push(row);
        puts("added " + inspect(row));
        process.nextTick(innerFunction);
      });
    })();
  }

  function onPrepare(error, statement) {
    puts("prep args " + inspect(arguments));
    if (bindings) {
      if (Object.prototype.toString.call(bindings) === "[object Array]") {
        doBindingsByIndex(statement, bindings, doStep);
      }
      else {
        // TODO index by keys
      }
    }
  }

  this.prepare(sql, onPrepare);
}

function SQLTransactionSync(db, txCallback, errCallback, successCallback) {
  this.database = db;

  this.rolledBack = false;

  this.executeSql = function(sqlStatement, arguments, callback) {
    if (this.rolledBack) return;
    var result = db.query(sqlStatement, arguments);
    if (callback) {
      var tx = this;
      callback.apply(result, [tx].concat(result.all));
    }
    return result;
  }

  var that = this;
  function unroll() {
    that.rolledBack = true;
  }

  db.addListener("rollback", unroll);

  this.executeSql("BEGIN TRANSACTION");
  txCallback(this);
  this.executeSql("COMMIT");

  db.removeListener("rollback", unroll);

  if (!this.rolledBack && successCallback)
    successCallback(this);
}


Database.prototype.transaction = function (txCallback, errCallback,
                                               successCallback) {
  var tx = new SQLTransactionSync(this, txCallback,
                                  errCallback, successCallback);
}

// TODO: readTransaction()
