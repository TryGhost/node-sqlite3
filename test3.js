var fs = require("fs");

process.mixin(GLOBAL, require("assert"));
process.mixin(GLOBAL, require("sys"));
var sqlite = require("./sqlite");

var db = new sqlite.Database();

// db.open("mydatabase.db", function () {
//   puts("opened the db");
//   db.query("SELECT * FROM foo WHERE baz < ? AND baz > ?", [6, 3], function (error, result) {
//     ok(!error);
//     puts("query callback " + inspect(result));
//     equal(result.length, 2);
//   });
// //   db.query("SELECT 1", function (error, result) {
// //     ok(!error);
// //     puts("query callback " + inspect(result));
// //     equal(result.length, 1);
// //   });
// });

function readTest(db) {
  var t0 = new Date;
  var count = i = 100;
  var rows = 0;
  var innerFunc = function () {
    if (!i--) {
      var d = ((new Date)-t0)/1000;
      puts("**** " + count + " selects in " + d + "s (" + (count/d) + "/s) "+rows+" rows total ("+(rows/d)+" rows/s)");
      return;
    }

    db.query("SELECT * FROM t1", function(error, results) {
      rows = rows + results.length;
      innerFunc();
    });
  };
  innerFunc();
}

function writeTest(db, callback) {
  var t0 = new Date;
  var count = i = 10000;
  var innerFunc = function () {
    if(!i--) {
      var d = ((new Date)-t0)/1000;
      puts("**** " + count + " insertions in " + d + "s (" + (count/d) + "/s)");

      callback(db);
      return;
    };

    db.query("INSERT INTO t1 VALUES (?);", [1], function() {
      innerFunc();
    });
  };

  innerFunc();
}

fs.unlink("speedtest.db", function () {
  db.open("speedtest.db", function () {
    puts(inspect(arguments));
    puts("open cb");

    db.query("CREATE TABLE t1 (alpha INTEGER)", function () {
      puts("create table callback" + inspect(arguments));
      writeTest(db, readTest);
    });
  });
});

