sys = require('sys');
fs = require('fs');
path = require('path');

TestSuite = require('async-testing/async_testing').TestSuite;
sqlite = require('sqlite3_bindings');

puts = sys.puts;
inspect = sys.inspect;

var name = "Fetching all results";
var suite = exports[name] = new TestSuite(name);

function createTestTable(db, callback) {
  db.prepare('CREATE TABLE table1 (id INTEGER, name TEXT, age FLOAT)',
    function (error, createStatement) {
      if (error) throw error;
      createStatement.step(function (error, row) {
        if (error) throw error;
        callback();
      });
    });
}

var testRows = [ [ 1, "foo",  9 ]
               , [ 2, "bar",  8 ]
               , [ 3, "baz",  7 ]
               , [ 4, "quux", 6 ]
               , [ 5, "juju", 5 ]
               ];

var testRowsExpected = [ { id: 5, name: 'juju', age: 5 }
                       , { id: 4, name: 'quux', age: 6 }
                       , { id: 3, name: 'baz', age: 7 }
                       , { id: 2, name: 'bar', age: 8 }
                       , { id: 1, name: 'foo', age: 9 }
                       ];

var tests = [
  { 'insert a row with lastinsertedid':
    function (assert, finished) {
      var self = this;

      self.db.open(':memory:', function (error) {
        function selectStatementPrepared(error, statement) {
          if (error) throw error;
          statement.fetchAll(function (error, rows) {
            if (error) throw error;

            assert.deepEqual(testRowsExpected, rows);

            self.db.close(function () {
              finished();
            });
          });
        }

        createTestTable(self.db,
          function () {
            function insertRows(db, rows, callback) {
              var i = rows.length;
              db.prepare('INSERT INTO table1 (id, name, age) VALUES (?, ?, ?)',
                function (error, statement) {
                  function doStep(i) {
                    statement.bindArray(rows[i], function () {
                      statement.step(function (error, row) {
                        if (error) throw error;
                        assert.ok(!row, "Row should be unset");
                        statement.reset();
                        if (i) {
                          doStep(--i);
                        }
                        else {
                          statement.finalize(function () {
                            callback();
                          });
                        }
                      });
                    });
                  }

                  doStep(--i);
                });
            }

            var selectSQL
                = 'SELECT * from table1';

            insertRows(self.db, testRows, function () {
              self.db.prepare(selectSQL
                            , selectStatementPrepared);
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
