/**
 *  issue:
 *  https://github.com/mapbox/node-sqlite3/issues/503
 */

var sqlite3 = require('..');
var assert = require('assert');

describe('finalize', function() {
    it('test Statement#finalize() with callback function', function(done) {
        var count = 10;

        var db = new sqlite3.Database(':memory:');
        db.serialize(function() {
            db.run("CREATE TABLE foo (id INT, value TEXT)");

            var stmt = db.prepare("INSERT INTO foo VALUES(?, ?)");
            for (var i = 0; i < count; i++) {
                stmt.run(i, 'Value for ' + i, function(err) {
                    if (err) throw err;
                });
            }
            stmt.finalize();

            var values = [];
            stmt = db.prepare("SELECT * FROM foo");
            stmt.each({}, function (err, row) {
                if (err) throw err;
                values.push(row.value);
            });
 
            stmt.finalize(function() {
                assert.equal(values.length, count);
                done();
            });

        });
    });
});
