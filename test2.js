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

db.open("mydatabase.db", function (err) {
  puts(inspect(arguments));
  if (err) {
    puts("There was an error");
    throw err;
  }

  puts("open callback");
  db.printIt();

  db.prepare("SELECT bar FROM foo; SELECT baz FROM foo;", function (error, statement) {
    puts(inspect(arguments));
    puts("prepare callback");
    db.prepare("SELECT bar FROM foo WHERE bar = 'hello'", function (error, statement) {
      puts("prepare callback");
      statement.bind(0, 666, function () {
        puts("bind callback");
      });
    });
  });
});

puts("done");
