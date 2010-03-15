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
var sqlite = require("./sqlite3_bindings");

var Database = exports.Database = function () {
  var self = this;
  this.queue = [];
  this.db = new sqlite.Database();
};

Database.prototype.dispatch = function () {
  if (!this.queue || this.currentQuery
                  || !this.queue.length) {
    return;
  }
  this.currentQuery = this.queue.shift();
  this.executeQuery.apply(this, this.currentQuery);
}

Database.prototype.open = function () {
  this.db.open.apply(this.db, arguments);
}

Database.prototype.close = function () {
  this.db.close.apply(this.db, arguments);
}

Database.prototype.prepare = function () {
  this.db.prepare.apply(this.db, arguments);
}

Database.prototype.query = function (sql, bindings, queryCallback) {
  this.queue = this.queue || [];
  this.queue.push([sql, bindings, queryCallback]);
  this.dispatch();
}

Database.prototype.executeQuery = function(sql, bindings, queryCallback) {
  var self = this;

  if (typeof(bindings) == "function") {
    var tmp = bindings;
    bindings = queryCallback;
    queryCallback = tmp;
  }

  // Iterate over the list of bindings. Since we can't use something as
  // simple as a for or while loop, we'll just chain them via the event loop
  function doBindingsByIndex(statement, bindings, queryCallback) {
    (function (statement, bindings, bindIndex) {
      var innerFunction = arguments.callee;
      if (!bindings.length) {
        process.nextTick(function () {
          queryCallback(statement);
        });
        return;
      }

      bindIndex = bindIndex || 1;
      var value = bindings.shift();

      process.nextTick(function () {
        statement.bind(bindIndex, value, function () {
          innerFunction(statement, bindings, bindIndex+1);
        });
      });
    })(statement, bindings, 1);
  }

  function queryDone(statement, rows) {
    if (statement.tail) {
      statement.finalize(function () {
        self.db.prepare(statement.tail, onPrepare);
      });
    }

    // if there are any queries queued, let them know it's safe to go
    self.db.emit("ready");
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
        process.nextTick(innerFunction);
      });
    })();
  }

  function onPrepare(error, statement) {
    if (bindings) {
      if (Object.prototype.toString.call(bindings) === "[object Array]") {
        doBindingsByIndex(statement, bindings, doStep);
      }
      else {
        // TODO index by keys
      }
    }
  }

  this.db.prepare(sql, onPrepare);
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
