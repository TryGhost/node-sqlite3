sys = require('sys');
fs = require('fs');
path = require('path');

TestSuite = require('async-testing/async_testing').TestSuite;
sqlite = require('sqlite3_bindings');

puts = sys.puts;
inspect = sys.inspect;

var name = "Caching of affectedRows";
var suite = exports[name] = new TestSuite(name);

function createTestTable(db, callback) {
  db.prepare('CREATE TABLE table1 (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT)',
    function (error, createStatement) {
      if (error) throw error;
      createStatement.step(function (error, row) {
        if (error) throw error;
        callback();
      });
    });
}

var tests = [
  { 'insert a row with lastinsertedid':
    function (assert, finished) {
      var self = this;

      self.db.open(':memory:', function (error) {
        function updateStatementCreated(error, statement) {
          if (error) throw error;
          statement.step(function (error, row) {
          if (error) throw error;
            assert.equal(this.affectedRows, 10, "Last inserted id should be 10");

            finished();
          });
        }

        createTestTable(self.db,
          function () {
            function insertRows(db, count, callback) {
              var i = count;
              db.prepare('INSERT INTO table1 (name) VALUES ("oh boy")',
                function (error, statement) {
                  statement.step(function (error, row) {
                    if (error) throw error;
                    assert.ok(!row, "Row should be unset");
                    statement.reset();
                    if (--i)
                      statement.step(arguments.callee);
                    else
                      callback();
                  });
                });
            }

            var updateSQL
                = 'UPDATE table1 SET name="o hai"';

            common.insertMany(self.db, 'table1',
                              ['name'],
                              [['o hai'],
                               ['o hai'],
                               ['o hai'],
                               ['o hai'],
                               ['o hai'],
                               ['o hai'],
                               ['o hai'],
                               ['o hai'],
                               ['o hai'],
                               ['o hai']],
                              function () {
              self.db.prepare(updateSQL
                              , { affectedRows: true }
                              , updateStatementCreated);
            });
          });
      });
    }
  }
];

// order matters in our tests
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
