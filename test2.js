var fs = require("fs");

process.mixin(GLOBAL, require("assert"));
process.mixin(GLOBAL, require("sys"));
var sqlite = require("./sqlite3_bindings");

var db = new sqlite.Database();

throws(function () {
  db.open();
});

throws(function () {
  db.open("my.db");
});

throws(function () {
  db.open("foo.db");
});

db.open("my.db", function (err) {
  puts(inspect(arguments));
  if (err) {
    puts("There was an error");
    throw err;
  }
  puts("open callback");
  db.printIt();

  db.prepare("SELECT foo FROM bar", function (statement) {
    puts("prepare callback");
  });
  db.prepare("SELECT foo FROM bar; SELECT bar FROM baz", function (statement) {
    puts("prepare callback");
  });
});
puts("done");
