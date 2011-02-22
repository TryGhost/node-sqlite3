var sqlite3 = require('sqlite3');
var assert = require('assert');

exports['test serialize() and parallelize()'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var inserted1 = 0;
    var inserted2 = 0;
    var retrieved = 0;

    var count = 1000;

    db.serialize();
    db.run("CREATE TABLE foo (txt text, num int, flt float, blb blob)");
    db.parallelize();

    var stmt1 = db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
    var stmt2 = db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
    for (var i = 0; i < count; i++) {
        stmt1.run('String ' + i, i, i * Math.PI, function(err) {
            if (err) throw err;
            inserted1++;
            // Might sometimes fail, but should work fine most of the time.
            assert.ok(inserted2 >= Math.floor(0.95 * inserted1));
        });
        i++;
        stmt2.run('String ' + i, i, i * Math.PI, function(err) {
            if (err) throw err;
            inserted2++;
            assert.ok(inserted1 >= Math.floor(0.95 * inserted2));
        });
    }
    db.serialize();
    db.all("SELECT txt, num, flt, blb FROM foo ORDER BY num", function(err, rows) {
        if (err) throw err;
        for (var i = 0; i < rows.length; i++) {
            assert.equal(rows[i][0], 'String ' + i);
            assert.equal(rows[i][1], i);
            assert.equal(rows[i][2], i * Math.PI);
            assert.equal(rows[i][3], null);
            retrieved++;
        }
    });

    beforeExit(function() {
        assert.equal(count, inserted1 + inserted2, "Didn't insert all rows");
        assert.equal(count, retrieved, "Didn't retrieve all rows");
    });
}

exports['test serialize(fn)'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var inserted = 0;
    var retrieved = 0;

    var count = 1000;
    db.serialize(function() {
        db.run("CREATE TABLE foo (txt text, num int, flt float, blb blob)");

        var stmt = db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
        for (var i = 0; i < count; i++) {
            stmt.run('String ' + i, i, i * Math.PI, function(err) {
                if (err) throw err;
                inserted++;
            });
        }

        db.all("SELECT txt, num, flt, blb FROM foo ORDER BY num", function(err, rows) {
            if (err) throw err;
            for (var i = 0; i < rows.length; i++) {
                assert.equal(rows[i][0], 'String ' + i);
                assert.equal(rows[i][1], i);
                assert.equal(rows[i][2], i * Math.PI);
                assert.equal(rows[i][3], null);
                retrieved++;
            }
        });
    });

    beforeExit(function() {
        assert.equal(count, inserted, "Didn't insert all rows");
        assert.equal(count, retrieved, "Didn't retrieve all rows");
    });
}
