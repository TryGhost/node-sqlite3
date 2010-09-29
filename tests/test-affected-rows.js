path = require('path');
require.paths.unshift(path.join(__dirname, 'lib'));
sys = require('sys');
fs = require('fs');

TestSuite = require('async-testing/async_testing').TestSuite;
sqlite = require('sqlite');

common = require('common');
puts = sys.puts;
inspect = sys.inspect;

var name = "Caching of affectedRows";
var suite = exports[name] = new TestSuite(name);

var tests = [
  { 'insert a row with lastinsertedid':
    function (assert, finished) {
      var self = this;

      var data = [ ['foo 0']
                 , ['foo 1']
                 , ['foo 2']
                 , ['foo 3']
                 , ['foo 4']
                 , ['foo 5']
                 , ['foo 6']
                 , ['foo 7']
                 , ['foo 8']
                 , ['foo 9']
                 ];

      common.createTable
        ( self.db
        , 'table1'
        , [ { name: 'id',   type: 'INT' }
          , { name: 'name', type: 'TEXT' }
          ]
        , function (error) {
            if (error) throw error;
            var updateSQL
                = 'UPDATE table1 SET name="o hai"';

            common.insertMany
              ( self.db
              , 'table1'
              , ['name']
              , data
              , function () {
                  self.db.prepare
                    ( updateSQL
                    , { affectedRows: true }
                    , onUpdateStatementCreated
                    );
                });
          }
        );

      function onUpdateStatementCreated(error, statement) {
        if (error) throw error;
        statement.step(function (error, row) {
          if (error) throw error;
          assert.equal
            ( this.affectedRows
            , data.length
            , "Last inserted id should be 10"
            );

          finished();
        });
      }
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
  this.db.open(':memory:', function (error) {
    finished();
  });
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
