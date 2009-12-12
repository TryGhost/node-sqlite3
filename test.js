// Test script for node_sqlite

var sys = require("sys");
var posix = require("posix");
var sqlite = require("./sqlite");

posix.unlink('test.db');

function asserteq(v1, v2) {
  if (v1 != v2) {
    sys.puts(sys.inspect(v1));
    sys.puts(sys.inspect(v2));
  }
  process.assert(v1 == v2);
}

var db = sqlite.openDatabaseSync('test.db');

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
  process.assert(as.length == 4);
  process.assert(ys.length == 4);
});

db.query("SELECT e FROM egg WHERE a = ?", [5], function (es) {
  asserteq(es.length, 1);
  asserteq(es[0].e, es.rows.item(0).e);
  asserteq(es[0].e, "E");
});



db.transaction(function(tx) {
  tx.executeSql("CREATE TABLE tex (t,e,x)");
  var i = tx.executeSql("INSERT INTO tex (t,e,x) VALUES (?,?,?)", 
                        ["this","is","Sparta"]);
  asserteq(i.rowsAffected, 1);
  var s = tx.executeSql("SELECT * FROM tex");
  asserteq(s.rows.length, 1);
  asserteq(s.rows.item(0).t, "this");
  asserteq(s.rows.item(0).e, "is");
  asserteq(s.rows.item(0).x, "Sparta");
});


db.query("CREATE TABLE test (x,y,z)", function () {
    db.query("INSERT INTO test (x,y) VALUES (?,?)", [5,10]);
    db.query("INSERT INTO test (x,y,z) VALUES ($x, $y, $z)", {$x:1, $y:2, $z:3});
  });

db.query("SELECT * FROM test WHERE rowid < ?;", [1]);

db.query("UPDATE test SET y = 10;", [], function () {
    sys.puts(this.rowsAffected + " rows affected");
    process.assert(this.rowsAffected == 2);
  });

db.transaction(function(tx) {
  tx.executeSql("SELECT * FROM test WHERE x = ?", [1], function (tx,records) {
    for (var i = 0; records.rows.item(i); ++i) 
      asserteq(records.rows.item(i).z, 3);
  });
});

var na = db.query("");
asserteq(na, null);
  
try {
  na = db.query("CRAPPY QUERY THAT DOESN'T WORK");
  asserteq("Apples", "Oranges");
} catch (e) {
}


db.close();
sys.puts("OK\n");
             
// Perhaps do this, one day
//var q = db.prepare("SELECT * FROM t WHERE rowid=?");
//var rows = q.execute([1]);
