// Test script for node_sqlite

var sys = require("sys");
var posix = require("posix");
var sqlite3 = require("./sqlite3");

posix.unlink('test.db');

var db = sqlite3.openDatabaseSync('test.db');

db.query("CREATE TABLE egg (a,y,e)");
db.query("INSERT INTO egg (a) VALUES (1)", function () {
  process.assert(this.insertId == 1);
});
var i2 = db.query("INSERT INTO egg (a) VALUES (?)", [5]);
process.assert(i2.insertId == 2);
db.query("UPDATE egg SET y='Y'; UPDATE egg SET e='E';");
db.query("UPDATE egg SET y=?; UPDATE egg SET e=? WHERE ROWID=1", 
         ["arm","leg"] );
db.query("INSERT INTO egg (a,y,e) VALUES (?,?,?)", [1.01, 10e20, -0.0]);
db.query("INSERT INTO egg (a,y,e) VALUES (?,?,?)", ["one", "two", "three"]);

db.query("SELECT * FROM egg", function (rows) {
    sys.puts(JSON.stringify(rows));
  });

db.query("SELECT a FROM egg; SELECT y FROM egg", function (as, ys) {
    sys.puts("As " + JSON.stringify(as));
    sys.puts("Ys " + JSON.stringify(ys));
  });

db.query("SELECT e FROM egg WHERE a = ?", [5], function (rows) {
    process.assert(rows[0].e == "E");
  });

db.query("CREATE TABLE test (x,y,z)", function () {
    db.query("INSERT INTO test (x,y) VALUES (?,?)", [5,10]);
    db.query("INSERT INTO test (x,y,z) VALUES ($x, $y, $z)", {$x:1, $y:2, $z:3});
  });

db.query("SELECT * FROM test WHERE rowid < ?;", [1],
         function (stmt) {
           sys.puts("rose:");
           sys.puts(stmt);
         });

db.query("UPDATE test SET y = 10;", [], function () {
    sys.puts(this.rowsAffected + " rows affected");
    process.assert(this.rowsAffected == 2);
  });

          
db.close();
sys.puts("OK\n");
process.exit()
             
// Perhaps do this, one day
//var q = db.prepare("SELECT * FROM t WHERE rowid=?");
//var rows = q.execute([1]);

// Perhaps use this syntax if query starts returning a promise:

c.query("select * from test;").addCallback(function (rows) {
  puts("result1:");
  p(rows);
});

c.query("select * from test limit 1;").addCallback(function (rows) {
  puts("result2:");
  p(rows);
});

c.query("select ____ from test limit 1;").addCallback(function (rows) {
  puts("result3:");
  p(rows);
}).addErrback(function (e) {
  puts("error! "+ e.message);
  puts("full: "+ e.full);
  puts("severity: "+ e.severity);
  c.close();
});


