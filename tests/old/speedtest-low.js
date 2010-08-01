var sqlite = require('../sqlite3_bindings');
var sys = require('sys');

var puts = sys.puts;
var inspect = sys.inspect;

var db = new sqlite.Database();

var total = 100000;
var rows = 0;
var t0;

function createTable(db, callback) {
  db.prepare("CREATE TABLE t1 (alpha INTEGER)", function (error, statement) {
    if (error) throw error;
    callback(); 
  });
}

function onPrepare() {
  var stash = arguments.callee.stash;
  insertValues(stash.db, stash.count--, stash.callback);
}

function insertValues(db, count, callback) {
  if (!count--) {
    var d = ((new Date)-t0)/1000;
    puts("**** " + d + "s to insert " + rows + " rows (" + (rows/d) + "/s)");

    if (callback) callback();
    return;
  }

  rows++;

  onPrepare.stash = {
    db: db,
    count: count,
    callback: callback
  };
  db.prepare("INSERT INTO t1 VALUES (1)", onPrepare)
}

db.open(':memory:', function () {
  createTable(db, function () {
    t0 = new Date();
    insertValues(db, total, function () {
    });
  });
});
