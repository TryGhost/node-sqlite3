var sqlite3 = require('sqlite3');
var assert = require('assert');
var fs = require('fs');

exports['test Database#exec'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');
    var finished = false;
    var sql = fs.readFileSync('test/support/script.sql', 'utf8');

    db.exec(sql, function(err) {
        if (err) throw err;

        db.all("SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY name", function(err, rows) {
            if (err) throw err;

            assert.ok(rows.length, 3);
            assert.deepEqual(rows, [
                { name: 'bar' },
                { name: 'baz' },
                { name: 'foo' }
            ]);

            finished = true;
        });
    });

    beforeExit(function() {
        assert.ok(finished);
    });
};
