var sqlite = require('sqlite3_bindings');
var sys = require('sys');
var assert = require('assert');

var puts = sys.puts;
var inspect = sys.inspect;

var db = new sqlite.Database();

var total = 100000;
var rows = 0;
var t0;

function getRows() {
  db.prepare("SELECT * FROM t1", function (error, statement) {
    if (error) throw error;
    rows = 0;
    t0 = new Date();

    function onStep(error, row) {
      var d;
      if (error) throw error;
      if (!row) {
        statement.finalize(function () { db.close(function () {}); });
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
  db.prepareAndStep("CREATE TABLE t1 (id INTEGER PRIMARY KEY, alpha INTEGER)", function (error, statement) {
    if (error) throw error;
    callback(); 
  });
}

var count = 0;

function onInsert(error, info) {
  var d;
  if (error) throw error;

  if (++rows == total) {
    d = ((new Date)-t0)/1000;
    puts("**** " + d + "s to insert " + rows + " rows (" + (rows/d) + "/s)");
    getRows();
  }
}

db.open(':memory:', function () {
  createTable(db, function () {
    t0 = new Date();
    for (var i = 0; i < total; i++) {
      db.prepareAndStep("INSERT INTO t1 (alpha) VALUES (1)", onInsert);
    }
  });
});
