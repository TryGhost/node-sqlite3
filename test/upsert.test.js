var sqlite3 = require('..');
var assert = require('assert');

describe('query properties', function() {
    var db;
    before(function(done) {
        db = new sqlite3.Database(':memory:');
        db.run("CREATE TABLE foo (id INT PRIMARY KEY, count INT)", done);
    });

    (sqlite3.VERSION_NUMBER < 3024000 ? it.skip : it)('should upsert', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?, ?)");
        stmt.run(1, 1, function(err) { // insert 1
            if (err) throw err;
            var upsert_stmt = db.prepare("INSERT INTO foo VALUES(?, ?) ON CONFLICT (id) DO UPDATE SET count = count + excluded.count");
            upsert_stmt.run(1, 2, function(err) { // add 2
                if (err) throw err;
                var select_stmt = db.prepare("SELECT count FROM foo WHERE id = ?");
                select_stmt.get(1, function(err, row) {
                    if (err) throw err;
                    assert.equal(row.count, 3); // equals 3
                });
            })
        });
        db.wait(done);
    });
});
