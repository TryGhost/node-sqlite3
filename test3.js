var fs = require("fs");

process.mixin(GLOBAL, require("assert"));
process.mixin(GLOBAL, require("sys"));
var sqlite = require("./sqlite");

var db = new sqlite.Database();

db.open("mydatabase.db", function () {
  puts("opened the db");
  db.query("SELECT * FROM foo WHERE baz < ? OR baz > ?; SELECT 1;", [6, 3], function (error, result) {
    puts(inspect(arguments));
    puts("query callback " + inspect(result));
  });
});
