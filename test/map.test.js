var sqlite3 = require('sqlite3');
var assert = require('assert');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test Database#map() with two columns'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var count = 10;
    var inserted = 0;
    var retrieved = false;

    db.serialize(function() {
       db.run("CREATE TABLE foo (id INT, value TEXT)");

       var stmt = db.prepare("INSERT INTO foo VALUES(?, ?)");
       for (var i = 5; i < count; i++) {
           stmt.run(i, 'Value for ' + i, function(err) {
               if (err) throw err;
               inserted++;
           });
       }
       stmt.finalize();

       db.map("SELECT * FROM foo", function(err, map) {
           retrieved = true;
           assert.deepEqual(map, { 5: 'Value for 5', 6: 'Value for 6', 7: 'Value for 7', 8: 'Value for 8', 9: 'Value for 9' });
       });

    });

    beforeExit(function() {
        assert.equal(inserted, 5);
        assert.ok(retrieved);
    });
};

exports['test Database#map() with three columns'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var count = 10;
    var inserted = 0;
    var retrieved = false;

    db.serialize(function() {
       db.run("CREATE TABLE foo (id INT, value TEXT, other TEXT)");

       var stmt = db.prepare("INSERT INTO foo VALUES(?, ?, ?)");
       for (var i = 5; i < count; i++) {
           stmt.run(i, 'Value for ' + i, null, function(err) {
               if (err) throw err;
               inserted++;
           });
       }
       stmt.finalize();

       db.map("SELECT * FROM foo", function(err, map) {
           retrieved = true;
           assert.deepEqual(map, {
               5: { id: 5, value: 'Value for 5', other: null },
               6: { id: 6, value: 'Value for 6', other: null },
               7: { id: 7, value: 'Value for 7', other: null },
               8: { id: 8, value: 'Value for 8', other: null },
               9: { id: 9, value: 'Value for 9', other: null }
           });
       });

    });

    beforeExit(function() {
        assert.equal(inserted, 5);
        assert.ok(retrieved);
    });
};
