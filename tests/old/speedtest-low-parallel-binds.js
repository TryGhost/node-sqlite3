var sqlite = require('../sqlite3_bindings');
var sys = require('sys');

var puts = sys.puts;
var inspect = sys.inspect;

var db = new sqlite.Database();

var total = 100000;
var rows = 0;
var t0;

function getRows() {
  db.prepare("SELECT alpha FROM t1", function (error, statement) {
    if (error) throw error;
    rows = 0;
    t0 = new Date();

    function onStep(error, row) {
      var d;
      if (error) throw error;
      if (!row) {
        statement.finalize(function () {  });
        d = ((new Date)-t0)/1000;
        puts("**** " + d + "s to fetch " + rows + " rows (" + (rows/d) + "/s)");
        return;
      }
      rows++;
      statement.step(onStep);
    }

    statement.step(onStep);
  });
}

function createTable(db, callback) {
  db.prepare("CREATE TABLE t1 (alpha INTEGER)", function (error, statement) {
    if (error) throw error;
    callback(); 
  });
}

var count = 0;

function onPrepare(error, statement) {
  var d;
   
  if (error) throw error;

  statement.bind(1, count++, function (error) {
    if (error) throw error;
    statement.step(function (row) {
      statement.finalize(function () {
        if (++rows == total) {
          d = ((new Date)-t0)/1000;
          puts("**** " + d + "s to insert " + rows + " rows (" + (rows/d) + "/s)");
          getRows();
        }
      });
    });
  });
}

db.open(':memory:', function () {
  createTable(db, function () {
    t0 = new Date();
    for (var i = 0; i < total; i++) {
      db.prepare("INSERT INTO t1 VALUES (?)", onPrepare);
    }
  });
});
