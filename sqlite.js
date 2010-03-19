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
  this.driver = new sqlite.Database();
  this.driver.addListener("ready", function () {
    self.dispatch();
  });
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
  this.driver.open.apply(this.driver, arguments);
}

Database.prototype.close = function () {
  this.driver.close.apply(this.driver, arguments);
}

Database.prototype.prepare = function () {
  this.driver.prepare.apply(this.driver, arguments);
}

Database.prototype.query = function (sql, bindings, queryCallback) {
  this.queue = this.queue || [];
  this.queue.push([sql, bindings, queryCallback]);
  this.dispatch();
}

// Iterate over the list of bindings. Since we can't use something as
// simple as a for or while loop, we'll just chain them via the event loop
function _setBindingsByIndex(db,
  statement, bindings, nextCallback, rowCallback, bindIndex) {

  if (!bindings.length) {
    nextCallback(db, statement, rowCallback);
    return;
  }

  bindIndex = bindIndex || 1;
  var value = bindings.shift();

  statement.bind(bindIndex, value, function () {
    _setBindingsByIndex(db, statement, bindings, nextCallback, rowCallback, bindIndex+1);
  });
}

function _queryDone(db, statement) {
  if (statement.tail) {
    statement.finalize(function () {
      db.driver.prepare(statement.tail, onPrepare);
    });
    return;
  }

  statement.finalize(function () {
    db.currentQuery = undefined;
    // if there are any queries queued, let them know it's safe to go
    db.driver.emit("ready");
  });
}

function _doStep(db, statement, rowCallback) {
  statement.step(function (error, row) {
    if (error) throw error;
    if (!row) {
//       rows.rowsAffected = this.changes();
//       rows.insertId = this.lastInsertRowid();
      rowCallback();
      _queryDone(db, statement);
      return;
    }
    rowCallback(row);
    _doStep(db, statement, rowCallback);
  });
}

function _onPrepare(db, statement, bindings, rowCallback) {
  if (Array.isArray(bindings)) {
    if (bindings.length) {
      _setBindingsByIndex(db, statement, bindings, _doStep, rowCallback);
    }
    else {
      _doStep(db, statement, rowCallback);
    }
  }
  else if (typeof(bindings) !== 'undefined') {
    // TODO index by keys
  }
}

Database.prototype.executeQuery = function(sql, bindings, rowCallback) {
  var self = this;

  if (typeof(bindings) == "function") {
    rowCallback = bindings;
    bindings = [];
  }

  this.driver.prepare(sql, function(error, statement) {
    if (error) throw error;
    if (statement) {
      _onPrepare(self, statement, bindings, rowCallback)
    } else {
      rowCallback();
      self.currentQuery = undefined;
      // if there are any queries queued, let them know it's safe to go
      self.dispatch();
    }
  });
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
