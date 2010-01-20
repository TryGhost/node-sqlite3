// Speed test for SQL. Based on http://www.sqlite.org/speed.html

var sys = require("sys");
var posix = require("posix");

try {
  posix.unlink("speedtest.db").wait();
} catch (e) {
  // Not there? That's okay.
}

function connect() {
  if (true) {
    // node-sqlite
    var sqlite = require("./sqlite");
    var db = sqlite.openDatabaseSync("speedtest.db");
  } else {
    // node-persistence
    // TODO: this doesn't work yet
    var sqlite = require("../../lib/node-persistence/lib/persistence/sqlite");
    var db = sqlite.new_connection("speedtest.db");
    db.executeSql = db.query;
    db.transaction = function(callback) {
      this.query("BEGIN TRANSACTION;");
      callback(this);
      this.query("COMMIT");
    }
  }
  return db;
}

function _rome(dn, ds) {
  if (n < dn) return false;
  n -= dn;
  s += ds;
  return true;
}

function toRoman(n) {
  if (isNaN(n)) return n;
  if (n <= 0) return "N";
  s = "";
  while (_rome(1000, "M")) {}
  _rome(900, "CM");
  _rome(500, "D");
  _rome(400, "CD");
  while (_rome(100, "C")) {}
  _rome(90, "XC");
  _rome(50, "L");
  _rome(40, "XL");
  while (_rome(10, "X")) {}
  _rome(9, "IX");
  _rome(5, "V");
  _rome(4, "IV");
  while (_rome(1, "I")) {}
  return s;
}

var db;

var SECTION;
var LASTTIME;
function time(section) {
  if (db) db.close();
  db = connect();
  var now = (new Date()).getTime();
  if (SECTION) {
    var elapsed = ((now - LASTTIME)/1000.0).toFixed(3);
    sys.puts(elapsed + ' ' + SECTION);
  }
  SECTION = section;
  LASTTIME = now;
}

time("Test 1: 1000 INSERTs");
db.query("CREATE TABLE t1(a INTEGER, b INTEGER, c VARCHAR(100));");
for (var i = 0; i < 1000; ++i) {
  var n = Math.floor(Math.random() * 99999);
  db.query("INSERT INTO t1 VALUES(?,?,?);", [i, n, toRoman(n)]);
}

time("Test 2: 25000 INSERTs in a transaction");
db.transaction(function (tx) {
  tx.executeSql("CREATE TABLE t2(a INTEGER, b INTEGER, c VARCHAR(100));");
  for (var i = 0; i < 25000; ++i) {
    var n = Math.floor(Math.random() * 99999);
    db.query("INSERT INTO t2 VALUES(?,?,?);", [i, n, toRoman(n)]);
  }
});

time("Test 3: 25000 INSERTs into an indexed table");
db.transaction(function (tx) {
  tx.executeSql("CREATE TABLE t3(a INTEGER, b INTEGER, c VARCHAR(100));");
  tx.executeSql("CREATE INDEX i3 ON t3(c);");
  for (var i = 0; i < 25000; ++i) {
    var n = Math.floor(Math.random() * 99999);
    db.query("INSERT INTO t3 VALUES(?,?,?);", [i, n, toRoman(n)]);
  }
});

time("Test 4: 100 SELECTs without an index");
db.transaction(function (tx) {
  for (var i = 0; i < 100; ++i) {
    db.query("SELECT count(*), avg(b) FROM t2 WHERE b>=? AND b<?;", 
             [i * 100, i * 100 + 1000]);
  }
});

time("Test 5: 100 SELECTs on a string comparison");
db.transaction(function (tx) {
  for (var i = 0; i < 100; ++i) {
    db.query("SELECT count(*), avg(b) FROM t2 WHERE c LIKE ?;",
             ['%' + toRoman(i) + '%']);
  }
});

time("Test 6: Creating an index");
db.query("CREATE INDEX i2a ON t2(a);");
db.query("CREATE INDEX i2b ON t2(b);");

time("Test 7: 5000 SELECTs with an index");
for (var i = 0; i < 5000; ++i) 
  db.query("SELECT count(*), avg(b) FROM t2 WHERE b>=? AND b<?;", 
           [i * 100, (i+1) * 100]);
  

time("Test 8: 1000 UPDATEs without an index");
db.transaction(function (tx) {
  for (var i = 0; i < 1000; ++i)
    tx.executeSql("UPDATE t1 SET b=b*2 WHERE a>=? AND a<?", 
                  [i * 10, (i+1) * 10]);
});

time("Test 9: 25000 UPDATEs with an index");
db.transaction(function (tx) {
  for (var i = 0; i < 25000; ++i) {
    var n = Math.floor(Math.random() * 1000000);
    tx.executeSql("UPDATE t2 SET b=? WHERE a=?", [n, i]);
  }
});

time("Test 10: 25000 text UPDATEs with an index");
db.transaction(function (tx) {
  for (var i = 0; i < 25000; ++i) {
    var n = Math.floor(Math.random() * 1000000);
    tx.executeSql("UPDATE t2 SET c=? WHERE a=?", [toRoman(n), i]);
  }
});

time("Test 11: INSERTs from a SELECT");
db.transaction(function (tx) {
  tx.executeSql("INSERT INTO t1 SELECT b,a,c FROM t2;");
  tx.executeSql("INSERT INTO t2 SELECT b,a,c FROM t1;");
});

time("Test 12: DELETE without an index");
db.query("DELETE FROM t2 WHERE c LIKE '%fifty%';");

time("Test 13: DELETE with an index");
db.query("DELETE FROM t2 WHERE a>10 AND a<20000;");

time("Test 14: A big INSERT after a big DELETE");
db.query("INSERT INTO t2 SELECT * FROM t1;");

time("Test 15: A big DELETE followed by 12000 small INSERTs");
db.transaction(function (tx) {
  tx.executeSql("DELETE FROM t1;");
  for (var i = 0; i < 12000; ++i) {
    var n = Math.floor(Math.random() * 100000);
    tx.executeSql("INSERT INTO t1 VALUES(?,?,?);", [i, n, toRoman(n)]);
  }
});

time("Test 16: DROP TABLE");
db.query("DROP TABLE t1;");
db.query("DROP TABLE t2;");
db.query("DROP TABLE t3;");

time();