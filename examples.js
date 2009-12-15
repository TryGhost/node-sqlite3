#!/usr/local/bin/node
/*
//!! To be processed by docbyex.py (or run by Node)

<head>
<title>node-sqlite</title>
<style>
  pre, code { color: #060; font-size: 11pt; }
  pre { margin-left: 2ex; padding: 1ex; background: #eee; }
  p { font-size: 12pt; }
  body { margin: 2em; background-color: #fff; color: black }
  h1,h2,h3,h4 { font-family: helvetica }
</style>
</head>

<body>
<h1>node-sqlite</h1>

<a href=http://sqlite.org/>SQLite</a> bindings for 
<a //href=http://nodejs.org/>Node</a>.

The semantics conform somewhat to those of the <a
href=http://dev.w3.org/html5/webdatabase/#sql>HTML5 Web SQL API</a>,
plus some extensions. Also, only the synchronous API is implemented;
the asynchronous API is a big TODO item.

<h2>Documentation by Example</h2>
*/

// Import the library and open a database. (Only syncronous database
// access is implemented at this time.)

var sqlite = require("./sqlite");
var db = sqlite.openDatabaseSync("example.db");

// Perform an SQL query on the database:

db.query("CREATE TABLE foo (a,b,c)");

// This is a more convenient form than the HTML5 syntax for the same
// thing, but which is also supported:

db.transaction(function(tx) {
  tx.executeSql("CREATE TABLE bar (x,y,z)");
});

// This allows the same or similar code to work on the client and
// server end (modulo browser support of HTML5 Web SQL).

// Transactions generate either a "commit" or "rollback" event.

var rollbacks = 0;
db.addListener("rollback", function () {
  ++rollbacks;
});

// Both forms take an optional second parameter which is values to
// bind to fields in the query, as an array:

db.query("INSERT INTO foo (a,b,c) VALUES (?,?,?)", ['apple','banana',22]);

// or as a map:

db.query("INSERT INTO bar (x,y,z) VALUES ($x,$y,$zebra)", 
         {$x: 10, $y:20, $zebra:"stripes"});

// Also optional is a callback function which is called with an object
// representing the results of the query:

db.query("SELECT x FROM bar", function (records) {
  process.assert(records.length == 1);
  process.assert(records[0].x == 10);

  // The HTML5 semantics for the record set also work:

  process.assert(records.rows.length == 1);
  process.assert(records.rows.item(0).x == 10);
});

// INSERT, UPDATE & DELETE queries set `rowsAffected` on their result
// set object:

db.query("UPDATE foo SET a = ? WHERE a = ?", ['orange', 'apple'], function(r) {
  process.assert(r.rowsAffected == 1);
});

// They also emit an `"update"` event.

// INSERT queries set `insertId`:

var insert = db.query("INSERT INTO foo VALUES (1,2,3)");
process.assert(insert.insertId == 2);

// Note here that the result set passed to the callback is also
// returned by `query`.

// Multiple-statement queries are supported; each statement's result set is retuned to the callback as a separate parameter:

var q = db.query("UPDATE bar SET z=20; SELECT SUM(z) FROM bar;",
                 function (update, select) {
                   process.assert(update.rowsAffected == 1);
                   process.assert(select[0]['SUM(z)'] == 20);
                 });

// An array of all result sets is available as the `.all` property on
// each result set:

process.assert(q.all[1].length == 1);

// HTML5 semantics are supported.

db.transaction(function(tx) {
  tx.executeSql("SELECT * FROM foo WHERE c = ?", [3], function(tx,res) {
    process.assert(res.rows.item(0).c == 3);
  });
});

// The `query` and `transaction` APIs wrap lower level APIs that more
// thinly wrap the underlying C api:

var stmt = db.prepare("INSERT INTO foo VALUES (?,?,?)");
stmt.bind(1, "curly");
stmt.bind(2, "moe");
stmt.bind(3, "larry");
stmt.step();  // Insert Curly, Moe & Larry
stmt.reset();
stmt.step();  // Insert another row with same stooges
stmt.reset();
stmt.clearBindings();
stmt.bind(2, "lonely");
stmt.step();  // Insert (null, "lonely", null)
stmt.finalize();

// Close it!

db.close();

// !!**
// Might as well clean up the mess we made.

var posix = require("posix");
posix.unlink('example.db');

var sys = require("sys");
sys.puts("OK");

