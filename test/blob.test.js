var sqlite3 = require('..'),
    fs = require('fs'),
    assert = require('assert'),
    Buffer = require('buffer').Buffer;

// lots of elmo
var elmo = fs.readFileSync(__dirname + '/support/elmo.png');

describe('blob', function() {
    var db;
    before(function(done) {
        db = new sqlite3.Database(':memory:');
        db.run("CREATE TABLE elmos (id INT, image BLOB)", done);
    });

    var total = 10;
    var inserted = 0;
    var retrieved = 0;


    it('should insert blobs', function(done) {
        for (var i = 0; i < total; i++) {
            db.run('INSERT INTO elmos (id, image) VALUES (?, ?)', i, elmo, function(err) {
                if (err) throw err;
                inserted++;
            });
        }
        db.wait(function() {
            assert.equal(inserted, total);
            done();
        });
    });

    it('should retrieve the blobs', function(done) {
        db.all('SELECT id, image FROM elmos ORDER BY id', function(err, rows) {
            if (err) throw err;
            for (var i = 0; i < rows.length; i++) {
                assert.ok(Buffer.isBuffer(rows[i].image));
                assert.ok(elmo.length, rows[i].image);

                for (var j = 0; j < elmo.length; j++) {
                    if (elmo[j] !== rows[i].image[j]) {
                        assert.ok(false, "Wrong byte");
                    }
                }

                retrieved++;
            }

            assert.equal(retrieved, total);
            done();
        });
    });

    it('should be able to select empty blobs', function(done) {
        const empty = new sqlite3.Database(':memory:');
        empty.serialize(function() {
            empty.run("CREATE TABLE files (id INTEGER PRIMARY KEY, data BLOB)");
            empty.run("INSERT INTO files (data) VALUES (X'')");
        });
        empty.get("SELECT data FROM files LIMIT 1", (err, row) => {
            if (err) throw err;
            assert.ok(Buffer.isBuffer(row.data));
            assert.equal(row.data.length, 0);
            empty.close(done);
        });
    })
});
