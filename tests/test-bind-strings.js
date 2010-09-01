require.paths.push(__dirname + '/..');

sys = require('sys');
fs = require('fs');
path = require('path');

TestSuite = require('async-testing/async_testing').TestSuite;
sqlite = require('sqlite');

// puts = sys.puts;
puts = function () {};

inspect = sys.inspect;

var name = "Test binding strings to placeholders";
var suite = exports[name] = new TestSuite(name);

function checkTable(db, assert, callback) {
  db.prepare('SELECT * FROM tbl WHERE foo="hello"', function (error, stmt) {
    puts("prepared2 ok", inspect(arguments));
    assert.ok(!error);
    stmt.step(function (error, row) {
      puts("step2 ok", inspect(arguments));
      assert.ok(!error);
      assert.ok(row, "We should get a row back");
      callback();
    });
  });
}

function setupTableWithBindArray(db, assert, callback) {
  db.prepareAndStep("CREATE TABLE tbl (foo TEXT)", function (error, stmt) {
    puts("created ok", inspect(arguments));
    assert.ok(!error);
    db.prepare("INSERT INTO tbl (foo) VALUES (?)", function (error, stmt) {
      assert.ok(!error);
      puts("prepared ok", inspect(arguments));
      stmt.bindArray(["hello"], function (error) {
        assert.ok(!error);
        checkTableCreated(stmt, assert, callback);
      });
    });
  });
}

function setupTableWithBindByKey(db, assert, callback) {
  db.prepareAndStep("CREATE TABLE tbl (foo TEXT)", function (error, stmt) {
    puts("created ok", inspect(arguments));
    assert.ok(!error);
    db.prepare("INSERT INTO tbl (foo) VALUES ($x)", function (error, stmt) {
      assert.ok(!error);
      puts("prepared ok", inspect(arguments));
      stmt.bind('$x', "hello", function (error) {
        assert.ok(!error);
        checkTableCreated(stmt, assert, callback);
      });
    });
  });
}

function setupTableWithBindByPosition(db, assert, callback) {
  db.prepareAndStep("CREATE TABLE tbl (foo TEXT)", function (error, stmt) {
    puts("created ok", inspect(arguments));
    assert.ok(!error);
    db.prepare("INSERT INTO tbl (foo) VALUES (?)", function (error, stmt) {
      assert.ok(!error);
      puts("prepared ok", inspect(arguments));
      stmt.bind(1, "hello", function (error) {
        assert.ok(!error);
        checkTableCreated(stmt, assert, callback);
      });
    });
  });
}

function checkTableCreated(stmt, assert, callback) {
  puts("bound", inspect(arguments));
  stmt.step(function (error, row) {
    assert.ok(!error);
    assert.ok(!row);
    puts("stepped", inspect(arguments));
    stmt.finalize(function () {
      callback();
    });
  });
}

var tests = [
  { "Check strings are bound ok using an array":
    function (assert, finished) {
      var self = this;
      self.db.open(":memory:", function () {
        setupTableWithBindArray(self.db, assert, function () {
          checkTable(self.db, assert, function () {
            finished();
          });
        });
      });
    }
  }
, { "Check strings are bound ok using keys":
    function (assert, finished) {
      var self = this;
      self.db.open(":memory:", function () {
        setupTableWithBindByKey(self.db, assert, function () {
          checkTable(self.db, assert, function () {
            finished();
          });
        });
      });
    }
  }
, { "Check strings are bound ok using placeholder position":
    function (assert, finished) {
      var self = this;
      self.db.open(":memory:", function () {
        setupTableWithBindByPosition(self.db, assert, function () {
          checkTable(self.db, assert, function () {
            finished();
          });
        });
      });
    }
  }
];

for (var i=0,il=tests.length; i < il; i++) {
  suite.addTests(tests[i]);
}

var currentTest = 0;
var testCount = tests.length;

suite.setup(function(finished, test) {
  this.db = new sqlite.Database();
  finished();
});
suite.teardown(function(finished) {
  if (this.db) this.db.close(function (error) {
                               finished();
                             });
  ++currentTest == testCount;
});

if (module == require.main) {
  suite.runTests();
}
