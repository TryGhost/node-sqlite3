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

  var db = new sqlite.Database();
  db.__proto__ = Database.prototype;

  return db;
};

Database.prototype = {
  __proto__: sqlite.Database.prototype,
  constructor: Database,
};

Database.prototype.query = function(sql, bindings, rowCallback) {
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

// Execute SQL statements separated by semi-colons.
// SQL must contain no placeholders. Results are discarded.
Database.prototype.executeScript = function (script, callback) {
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
