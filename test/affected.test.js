var sqlite3 = require('sqlite3');
var assert = require('assert');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test row changes and lastID'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var finished = false;

    db.serialize(function() {
        db.run("CREATE TABLE foo (id INT, txt TEXT)");
        var stmt = db.prepare("INSERT INTO foo VALUES(?, ?)");
        var j = 1;
        for (var i = 0; i < 1000; i++) {
            stmt.run(i, "demo", function(err) {
                if (err) throw err;
                // Relies on SQLite's row numbering to be gapless and starting
                // from 1.
                assert.equal(j++, this.lastID);
            });
        }

        db.run("UPDATE foo SET id = id + 1 WHERE id % 2 = 0", function(err) {
            if (err) throw err;
            assert.equal(500, this.changes);
            finished = true;
        });
    });

    beforeExit(function() {
        assert.ok(finished);
    })
}
