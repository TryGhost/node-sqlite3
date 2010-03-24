var sqlite = require('./sqlite3_bindings');
var sys = require('sys');

var puts = sys.puts;
var inspect = sys.inspect;

var db = new sqlite.Database();

var total = 100000;
var rows = 0;
var t0;

function onStep () {
  puts("got a row back");
}

function getRows() {
  db.prepare("SELECT * FROM t1", function (error, statement) {
    if (error) throw error;
    
    for (var i = 0; i < 5; i++) {
      statement.step(onStep);
    }
  });
}

function createTable(db, callback) {
  db.prepare("CREATE TABLE t1 (alpha INTEGER)", function (error, statement) {
    if (error) throw error;
    callback(); 
  });
}

function onPrepare() {
  var d;
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
      db.prepare("INSERT INTO t1 VALUES (1)", onPrepare);
    }
  });
});
