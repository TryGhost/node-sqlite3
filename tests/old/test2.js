var fs = require("fs");
var sys = require("sys");
var assert = require("assert");

var sqlite = require("../sqlite3_bindings");

var puts = sys.puts;
var inspect = sys.inspect;

var db = new sqlite.Database();

assert.throws(function () {
  db.open();
});

assert.throws(function () {
  db.open("my.db");
});

assert.throws(function () {
  db.open("foo.db");
});

function test_prepare() {
  db.open("mydatabase.db", function (err) {
    puts(inspect(arguments));
    if (err) {
      puts("There was an error");
      throw err;
    }

    puts("open callback");
    db.printIt();

    db.prepare("SELECT bar FROM foo; SELECT baz FROM foo;", function (error, statement) {
      puts("prepare callback");
      db.prepare("SELECT bar FROM foo WHERE bar = $x and (baz = $y or baz = $z)", function (error, statement) {
        test_bind();
      });

    });
  });
}

function test_bind() {
  db.prepare("SELECT bar FROM foo WHERE bar = $x and (baz = $y or baz = $z)", function (error, statement) {
    statement.bind(0, 666, function () {
      puts("bind callback");

      statement.bind('$y', 666, function () {
      puts("bind callback");

      statement.bind('$x', "hello", function () {
        puts("bind callback");

        statement.bind('$z', 3.141, function () {
          puts("bind callback");

          //               statement.bind('$a', 'no such param', function (err) {
          //                 puts(inspect(arguments));
          puts("bind callback");
          statement.step(function () {
              puts("step callback");
              });
          //               });
          });
        });
      });
    });
    puts("prepare callback");
  });
}

function test_simple() {
  db.open("mydatabase.db", function () {
    db.prepare("SELECT bar, baz FROM foo WHERE baz > 5", function (error, statement) {
      puts('prepare callback');
      if (error) throw (error);  
      puts(inspect(arguments));

      statement.step(function (error) {

        puts('query callback');
        puts(inspect(arguments));
        statement.step(function () {
          puts('query callback');
          puts(inspect(arguments));
          statement.finalize(function () {
            puts("finalize callback");
            db.close(function () {
              puts("closed database");
              puts("close args " + inspect(arguments));
            });
          });
        });
      });
    });
  });
}

function test_close() {
  db.open("mydatabase.db", function () {
    db.close(function () {
      puts("closed database");
    });
  });
}

// test_close();
test_simple();
