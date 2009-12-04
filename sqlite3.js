var sys = require("sys");
var bindings = require("./sqlite3_bindings");

var Db = bindings.Db;

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

exports.Db = Db;