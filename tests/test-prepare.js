sys = require('sys');
fs = require('fs');
path = require('path');

TestSuite = require('async-testing/async_testing').TestSuite;
sqlite = require('sqlite3_bindings');

puts = sys.puts;
inspect = sys.inspect;

var name = "SQLite bindings";
var suite = exports[name] = new TestSuite(name);

function createTestTable(db, callback) {
  db.prepare('CREATE TABLE test1 (column1 TEXT)',
    function (error, createStatement) {
      if (error) throw error;
      createStatement.step(function (error, row) {
        if (error) throw error;
        callback();
      });
    });
}

function fetchAll(db, sql, callback) {
  db.prepare(
    sql,
    function (error, statement) {
      if (error) throw error;
      var rows = [];
      function doStep() {
        statement.step(function (error, row) {
          if (error) throw error;
          if (row) {
            rows.push(row);
            doStep();
          }
          else {
            callback(rows);
          }
        });
      }
      doStep();
    });
}

var tests = [
  { 'create in memory database object': 
    function (assert, finished) {
      this.db.open(':memory:', function (error) {
        assert.ok(!error)
        finished();
      });
    }
  }
, { 'prepareAndStep() a statement that returns no rows': 
    function (assert, finished) {
      var self = this;
      this.db.open(':memory:', function (error) {
        if (error) throw error;

        // use a query that will never return rows
        self.db.prepareAndStep('SELECT * FROM (SELECT 1) WHERE 1=0',
          function (error, statement) {
            if (error) throw error;
            assert.equal(error, undefined)

            // no statement will be returned here
            assert.ok(!statement)
            finished();
          });
      });
    }
  }
, { 'prepareAndStep (and step over) a statement that returns a row': 
    function (assert, finished) {
      var self = this;
      this.db.open(':memory:', function (error) {
        if (error) throw error;

        // use a query that will never return rows
        self.db.prepareAndStep('SELECT 1 AS handbanana',
          function (error, statement) {
            if (error) throw error;
            assert.equal(error, undefined)

            assert.ok(statement)
            var rows = [];
            
            // iterate over rows
            function doStep() {
              statement.step(function (error, row) {
                if (error) throw error;
                if (row) {
                  rows.push(row);
                  doStep();
                }
                else {
                  assert.deepEqual(rows, [{ handbanana: '1' }]);
                  finished();
                }
              });
            }
            doStep();
          });
      });
    }
  }
, { 'prepare (and step over) a statement that returns a row': 
    function (assert, finished) {
      var self = this;
      this.db.open(':memory:', function (error) {
        if (error) throw error;

        // use a query that will never return rows
        self.db.prepare('select 1 AS handbanana',
          function (error, statement) {
            if (error) throw error;
            assert.equal(error, undefined)

            assert.ok(statement)
            var rows = [];
            
            // iterate over rows
            function doStep() {
              statement.step(function (error, row) {
                if (error) throw error;
                if (row) {
                  rows.push(row);
                  doStep();
                }
                else {
                  assert.deepEqual(rows, [{ handbanana: '1' }]);
                  finished();
                }
              });
            }
            doStep();
          });
      });
    }
  }
, { 'prepare a statement that returns no result with prepare()': 
    function (assert, finished) {
      var self = this;
      this.db.open(':memory:', function (error) {
        if (error) throw error;
        self.db.prepare('select * from (select 1) where 1=0;',
          function (error, statement) {
            if (error) throw error;
            assert.equal(error, undefined)
            
            // we'll get a prepared statement here
            assert.ok(statement)
            finished();
          });
      });
    }
  }
, { 'insert a few times using step and reset': 
    function (assert, finished) {
      var self = this;
      self.db.open(':memory:', function (error) {
        createTestTable(self.db,
          function () {
            self.db.prepare('INSERT INTO test1 (column1) VALUES (1)',
              function (error, insertStatement) {
                if (error) throw error;

                var i = 0;
                function doStep() {
                  insertStatement.step(function (error, row) {
                    if (error) throw error;
                    if (row) {
                      doStep();
                    }
                    else {
                      if (++i < 5) {
                        insertStatement.reset();
                        doStep();
                      }
                      else {
                        fetchAll(self.db, "SELECT * FROM test1",
                          function (rows) {
                            assert.deepEqual(rows, [
                                { column1: '1' }
                              , { column1: '1' }
                              , { column1: '1' }
                              , { column1: '1' }
                              , { column1: '1' }
                              ]);
                            finished();
                          });
                      }
                    }
                  });
                }

                doStep();
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
