var fs = require("fs");

process.mixin(GLOBAL, require("assert"));
process.mixin(GLOBAL, require("sys"));
var sqlite = require("./sqlite");

var db = new sqlite.Database();

db.open("mydatabase.db", function () {
  puts("opened the db");
  db.query("SELECT * FROM foo WHERE baz < ? AND baz > ?", [6, 3], function (error, result) {
    ok(!error);
    puts("query callback " + inspect(result));
    equal(result.length, 2);
  });
  db.query("SELECT 1", function (error, result) {
    ok(!error);
    puts("query callback " + inspect(result));
    equal(result.length, 1);
  });
});
