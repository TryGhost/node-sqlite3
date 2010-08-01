var fs     = require("fs"),
    sys    = require("sys"),
    sqlite = require("../sqlite");

var puts = sys.puts;
var inspect = sys.inspect;

var db = new sqlite.Database();

function readTest(db, callback) {
  var t0 = new Date;
  var rows = 0;
  db.query("SELECT * FROM t1", function(error, row) {
    if (!row) {
      var d = ((new Date)-t0)/1000;
      puts("**** " + rows + " rows in " + d + "s (" + (rows/d) + "/s)");

      if (callback) callback(db);

    } else {
//       puts("got a row" + inspect(arguments));
      rows++;
    }
  });
}

function writeTest(db, i, callback) {
  db.insert("INSERT INTO t1 VALUES (1)", function (error, id) {
    if (!i--) {
      // end of results
      var dt = ((new Date)-t0)/1000;
      puts("**** " + count + " insertions in " + dt + "s (" + (count/dt) + "/s)");

      if (callback) callback(db);

    } else {
      writeTest(db, i--, callback);
    }
  });
}

var count = 100000;
var t0;

db.open(":memory:", function () {
  db.query("CREATE TABLE t1 (alpha INTEGER)", function () {
    puts("create table callback" + inspect(arguments));
//     writeTest(db, readTest);
    t0 = new Date;
    writeTest(db, count, readTest);
  });
});
