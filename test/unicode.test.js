var sqlite3 = require('..');
var assert = require('assert');

function randomString() {
    var str = '';
    for (var i = Math.random() * 300; i > 0; i--) {
        str += String.fromCharCode(Math.floor(Math.random() * 65536));
    }
    return str;
}

describe('unicode', function() {
    var db;
    before(function(done) { db = new sqlite3.Database(':memory:', done); });

        // Generate random data.
    var data = [];
    var length = Math.floor(Math.random() * 1000) + 200;
    for (var i = 0; i < length; i++) {
        data.push(randomString());
    }

    var inserted = 0;
    var retrieved = 0;

    it('should create the table', function(done) {
        db.run("CREATE TABLE foo (id int, txt text)", done);
    });

    it('should insert all values', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?, ?)");
        for (var i = 0; i < data.length; i++) {
            stmt.run(i, data[i], function(err) {
                if (err) throw err;
                inserted++;
            });
        }
        stmt.finalize(done);
    });

    it('should retrieve all values', function(done) {
        db.all("SELECT txt FROM foo ORDER BY id", function(err, rows) {
            if (err) throw err;

            for (var i = 0; i < rows.length; i++) {
                assert.equal(rows[i].txt, data[i]);
                retrieved++;
            }
            done();
        });
    });

    it('should have inserted and retrieved the correct amount', function() {
        assert.equal(inserted, length);
        assert.equal(retrieved, length);
    });

    after(function(done) { db.close(done); });
});
