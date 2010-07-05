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

// load numeric constants from sqlite3_bindings
for (prop in sqlite) {
    var obj = sqlite[prop];
    if ((obj === +obj) || (toString.call(obj) === '[object Number]') ) {
        exports[prop] = sqlite[prop];
    }
}

var Database = exports.Database = function () {
  var self = this;

  var db = new sqlite.Database();
  db.__proto__ = Database.prototype;

  return db;
};

Database.prototype = {
  __proto__: sqlite.Database.prototype,
  constructor: Database,
};

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
      db.prepare(statement.tail, onPrepare);
    });
    return;
  }

  statement.finalize(function () {
    // if there are any queries queued, let them know it's safe to go
    db.emit("ready");
  });
}

function _doStep(db, statement, rowCallback) {
  statement.step(function (error, row) {
    if (error)
        return rowCallback (error);

    if (!row) {
      rowCallback();
      _queryDone(db, statement);
      return;
    }
    rowCallback(undefined, row);

    _doStep(db, statement, rowCallback);
  });
}

function _onPrepare(db, statement, bindings, rowCallback) {
  function next() {
    _doStep(db, statement, rowCallback);
  }
  if (Array.isArray(bindings)) {
    statement.bindArray(bindings, next);
  }
  else if (typeof(bindings) === 'object') {
    statement.bindObject(bindings, next);
  }
}

Database.prototype.query = function(sql, bindings, rowCallback, prepareMode) {
  var self = this;

  if (typeof(bindings) == "function") {
    prepareMode = rowCallback || sqlite.EXEC_EMPTY;
    rowCallback = bindings;
    bindings = [];
  }
  if (typeof(prepareMode) == "undefined")
    prepareMode = sqlite.EXEC_EMPTY;

  this.prepare(sql, function(error, statement) {
    if (error)
        return rowCallback (error);
    if (statement) {
      _onPrepare(self, statement, bindings, rowCallback);
    } else {
      rowCallback();
    }
  }, prepareMode);
}

Database.prototype.insert = function(sql, insertCallback) {
  var self = this;

  this.prepare(sql, function(error, info) {
    if (error)
        return insertCallback (error);

    insertCallback (undefined, (info ? info.last_inserted_id : 0));
  }, sqlite.EXEC_LAST_INSERT_ID);
}

