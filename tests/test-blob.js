require.paths.push(__dirname + '/..');

sys = require('sys');
fs = require('fs');
path = require('path');

TestSuite = require('async-testing/async_testing').TestSuite;
sqlite = require('sqlite');

var db = new sqlite.Database();

var name = "Reading and writing blobs";
var suite = exports[name] = new TestSuite(name);
var fixture_path = path.join(__dirname, 'fixtures/principia_mathematica.jpg');

function createTestTable(db, callback) {
  db.prepare('CREATE TABLE [images] (name text,image blob);',
    function (error, createStatement) {
      if (error) throw error;
      createStatement.step(function (error, row) {
        if (error) throw error;
        callback();
      });
    });
}

var tests  = [
  { "Blob read/write":
    function(assert, finished) {
      var that = this;
      this.db.open(':memory:', function() {
        createTestTable(that.db, function() {
          that.db.prepare(
            'INSERT INTO images (name, image) VALUES ("principia_mathematica", ?);',
            function(err, statement) {
              if (err) throw err;
              fs.readFile(fixture_path, function(err, data) {
                if (err) throw err;
                assert.equal(data.length, 48538);
                statement.bind(1, data, function(err) {
                  if (err) throw err;
                  statement.step(function(err, row) {
                    if (err) throw err;
                    that.db.prepare(
                      'SELECT image FROM images WHERE name = "principia_mathematica";',
                      function(err, statement) {
                        if (err) throw err;
                        statement.step(function(err, row) {
                          if (err) throw err;
                          assert.equal(row.image.length, 48538);
                          finished();
                        });
                      }
                    );
                  });
                });
              });
            }
          );
        });
      });
    }
  },
  { "Close database after blob query":
    function(assert, finished) {
      this.db.close(function (err) {
        if (err) throw err;
        finished();
      });
    }
  }
];

for (var i=0,il=tests.length; i < il; i++) {
  suite.addTests(tests[i]);
}

var currentTest = 0;
var testCount = tests.length;

suite.db = new sqlite.Database();
suite.setup(function(finished, test) {
  this.db = suite.db;
  finished();
});

suite.teardown(function(finished) {
  finished();
  ++currentTest == testCount;
});

if (module == require.main) {
  suite.runTests();
}


