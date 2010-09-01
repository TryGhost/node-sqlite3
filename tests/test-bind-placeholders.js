require.paths.push(__dirname + '/..');

sys = require('sys');
fs = require('fs');
path = require('path');

TestSuite = require('async-testing/async_testing').TestSuite;
sqlite = require('sqlite3_bindings');

puts = sys.puts;
inspect = sys.inspect;

var name = "Binding statement place holders";
var suite = exports[name] = new TestSuite(name);

var tests = [
  { "Bind placeholders by position":
    function (assert, finished) {
      var self = this;
      self.db.open(":memory:", function () {
        self.db.prepare("SELECT ? AS foo, ? AS bar, ? AS baz"
                  , function (error, statement) {
          if (error) throw error;
          statement.bind(1, "hi", function (error) {
            if (error) throw error;
            statement.bind(2, "there", function (error) {
              if (error) throw error;
              statement.bind(3, "world", function (error) {
                if (error) throw error;
                statement.step(function (error, row) {
                  if (error) throw error;
                  assert.deepEqual(row
                                 , { foo: 'hi'
                                   , bar: 'there'
                                   , baz: 'world'
                                   });
                  finished();
                });
              });
            });
          });
        });
      });
    }
  }
, { "Bind placeholders using an array":
    function (assert, finished) {
      var self = this;
      self.db.open(":memory:", function () {
        self.db.prepare("SELECT ? AS foo, ? AS bar, ? AS baz"
                 , function (error, statement) {
          if (error) throw error;

          statement.bindArray(['hi', 'there', 'world'], function (error) {
            if (error) throw error;

            statement.step(function (error, row) {
              if (error) throw error;

              assert.deepEqual(row
                             , { foo: 'hi'
                               , bar: 'there'
                               , baz: 'world'
                               });
              statement.reset();
              statement.bindArray([1, 2, null], function (error) {
                statement.step(function (error, row) {
                  if (error) throw error;
                  assert.deepEqual(row, { foo: 1, bar: 2, baz: null });
                  finished();
                });
              });
            });
          });
        });
      });
    }
  }
, { "Bind placeholders using an object":
    function (assert, finished) {
      var self = this;
      self.db.open(":memory:", function () {
        self.db.prepare("SELECT $x AS foo, $y AS bar, $z AS baz"
                  , function (error, statement) {
          if (error) throw error;
          statement.bindObject({ $x: 'hi', $y: null, $z: 'world' }, function (error) {
            if (error) throw error;
            statement.step(function (error, row) {
              if (error) throw error;
              assert.deepEqual(row, { foo: 'hi', bar: null, baz: 'world' });
              statement.reset();
              statement.bindArray([1, 2, null], function (error) {
                statement.step(function (error, row) {
                  if (error) throw error;
                  assert.deepEqual(row, { foo: 1, bar: 2, baz: null });
                  finished();
                });
              });
            });
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
