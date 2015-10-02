var sqlite3 = require('..');
var assert = require('assert');
var fs = require('fs');

describe('sync', function() {
    var db;
    before(function(done) {
        db = new sqlite3.Database(':memory:', done);
    });

    it('Database#runSync', function(done) {
        db.runSync("CREATE TABLE foo (id INT, txt TEXT)");
        db.runSync("INSERT INTO foo VALUES(1, ?);", "bar");

        db.get('SELECT id, txt FROM foo LIMIT 1', function (err, row) {
            assert.equal(row['id'], 1);
            assert.equal(row['txt'], 'bar');
            done();
        });
    });
});
