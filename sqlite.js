/*
Copyright (c) 2009, Eric Fredricksen <e@fredricksen.net>

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

// TODO: async

var sys = require("sys");
var bindings = require("./sqlite3_bindings");
process.mixin(GLOBAL, bindings);
process.mixin(exports, bindings);


// Conform somewhat to http://dev.w3.org/html5/webdatabase/#sql


exports.openDatabaseSync = function (name, version, displayName, 
                                     estimatedSize, creationCallback) {
  var db = new DatabaseSync(name);
  if (creationCallback) creationCallback(db);
  return db;
}


// This is an extension of the HTML5 API
DatabaseSync.prototype.query = function (sql, bindings, callback) {
  // TODO: error callback
  if (typeof(bindings) == "function") {
    var tmp = bindings;
    bindings = callback;
    callback = tmp;
  }
  var result = this.performQuery(sql, bindings);
  if (typeof(callback) == "function") {
    callback.apply(result, result);
  }
  return result;
}



// TODO: void *sqlite3_commit_hook(sqlite3*, int(*)(void*), void*);
// TODO: void *sqlite3_rollback_hook(sqlite3*, void(*)(void *), void*);


function SQLTransactionSync(db, txCallback, errCallback, successCallback) {
  this.database = db;
  this.executeSql = function(sqlStatement, arguments, callback) {
    var result = db.query(sqlStatement, arguments, callback)[0];
    result.rows = {item: function (index) { return result[index]; },
                   length: result.length};
    return result;
  }

  db.query("BEGIN TRANSACTION");
  txCallback(this);
  db.query("COMMIT");
  if (successCallback) successCallback(this);
}


DatabaseSync.prototype.transaction = function (txCallback, errCallback, 
                                               successCallback) {
  var tx = new SQLTransactionSync(this, txCallback, errCallback, successCallback);
}

// TODO: readTransaction()

