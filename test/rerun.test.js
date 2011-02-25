var sqlite3 = require('sqlite3');
var assert = require('assert');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test running the same statement multiple times'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var count = 10;
    var inserted = 0;
    var retrieved = 0;

    db.serialize(function() {
       db.run("CREATE TABLE foo (id int)");

       var stmt = db.prepare("INSERT INTO foo VALUES(?)");
       for (var i = 5; i < count; i++) {
           stmt.run(i, function(err) {
               if (err) throw err;
               inserted++;
           });
       }
       stmt.finalize();

       var collected = [];
       var stmt = db.prepare("SELECT id FROM foo WHERE id = ?");
       for (var i = 0; i < count; i++) {
           stmt.get(i, function(err, row) {
               if (err) throw err;
               if (row) collected.push(row);
           });
       }
       stmt.finalize(function() {
           retrieved += collected.length;
           assert.deepEqual(collected, [ { id: 5 }, { id: 6 }, { id: 7 }, { id: 8 }, { id: 9 } ]);
       });
    });

    beforeExit(function() {
        assert.equal(inserted, 5);
        assert.equal(retrieved, 5);
    });
};
