sys = require('sys');
fs = require('fs');
path = require('path');
common = require('./lib/common');

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
               , [ 3, null,  7 ]
               , [ 4, "quux", 6 ]
               , [ 5, "juju", null ]
               ];

var testRowsExpected = [ { id: 5, name: 'juju', age: null }
                       , { id: 4, name: 'quux', age: 6 }
                       , { id: 3, name: null, age: 7 }
                       , { id: 2, name: 'bar', age: 8 }
                       , { id: 1, name: 'foo', age: 9 }
                       ];

var tests = [
  { 'insert a row with lastinsertedid':
    function (assert, finished) {
      var self = this;

      self.db.open(':memory:', function (error) {

        createTestTable(self.db,
          function () {
            common.insertMany(self.db
                            , 'table1'
                            , ['id', 'name', 'age']
                            , testRows
                            , function () {
              common.getResultsStep(self.db, function (rows) {
                console.log(sys.inspect(arguments));
                assert.deepEqual(rows, testRowsExpected);
                finished();
              });
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
