var sys = require("sys");
var bindings = require("./sqlite3_bindings");

var Db = bindings.Db;
exports.Db = Db;

Db.prototype.query = function (sql, bindings, callback) {
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


// Conform to http://dev.w3.org/html5/webdatabase/#sql

var DatabaseSync = Db;

exports.openDatabaseSync = function (name, version, displayName, 
                                     estimatedSize, creationCallback) {
  var db = new DatabaseSync(name);
  if (creationCallback) creationCallback(db);
  return db;
}


// TODO: slightly differnt args
Db.prototype.executeSql = Db.prototype.query;


// TODO: (big) async

// TODO: void *sqlite3_commit_hook(sqlite3*, int(*)(void*), void*);
// TODO: void *sqlite3_rollback_hook(sqlite3*, void(*)(void *), void*);


function SQLTransactionSync(db, txCallback, errCallback, successCallback) {
  this.database = db;
  this.executeSql = function(sqlStatement, argumets) {
    return db.query(sqlStatement, arguments)[0];
  }

  this.query("BEGIN TRANSACTION");
  txCallback(this);
  this.query("COMMIT");
  if (sucessCallback) successCallback(this);
}


Db.prototype.transaction = function (txCallback, errCallback, successCallback) {
  var tx = new Transaction(this, txCallback, errCallback, successCallback);
}

// TODO: readTransaction

