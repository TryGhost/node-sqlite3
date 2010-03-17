var fs = require("fs");

process.mixin(GLOBAL, require("assert"));
process.mixin(GLOBAL, require("sys"));
var sqlite = require("./sqlite");

var db = new sqlite.Database();

function readTest(db, callback) {
  var t0 = new Date;
  var rows = 0;
  db.query("SELECT * FROM t1", function(row) {
    if (!row) {
      var d = ((new Date)-t0)/1000;
      puts("**** " + rows + " rows in " + d + "s (" + (rows/d) + "/s)");

      if (callback) callback(db);
    }
    else {
      rows++;
    }
  });
}

function writeTest(db, callback) {
  var t0 = new Date;
  var count = i = 100000;

  function innerFunction () {
    db.query("INSERT INTO t1 VALUES (1)", function (row) {
      if (!i--) {
        // end of results
        var dt = ((new Date)-t0)/1000;
        puts("**** " + count + " insertions in " + dt + "s (" + (count/dt) + "/s)");

        if (callback) callback(db);
      }
      else {
        innerFunction();
      }
    });
  }
  innerFunction();
}

db.open(":memory:", function () {
  puts(inspect(arguments));
  puts("open cb");

  db.query("CREATE TABLE t1 (alpha INTEGER)", function () {
    puts("create table callback" + inspect(arguments));
    writeTest(db, readTest);
  });
});
