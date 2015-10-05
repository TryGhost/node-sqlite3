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
        db.runSync("INSERT INTO foo VALUES(1, ?), (2, ?);", "bar", "baz");

        db.get('SELECT id, txt FROM foo ORDER BY id ASC LIMIT 1', function (err, row) {
            assert.equal(row['id'], 1);
            assert.equal(row['txt'], 'bar');
            done();
        });
    });

    it('Database#getSync', function(done) {
        var row = db.getSync('SELECT id, txt FROM foo ORDER BY id ASC LIMIT 1');
        assert.equal(row['id'], 1);
        assert.equal(row['txt'], 'bar');

        done();
    });

    it('Database#allSync', function(done) {
        var rows = db.allSync('SELECT id, txt FROM foo ORDER BY id ASC');
        assert.equal(rows[0]['id'], 1);
        assert.equal(rows[0]['txt'], 'bar');
        assert.equal(rows[1]['id'], 2);
        assert.equal(rows[1]['txt'], 'baz');

        done();
    });
});
