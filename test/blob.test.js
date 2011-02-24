var sqlite3 = require('sqlite3'),
    Step = require('step'),
    fs = require('fs'),
    assert = require('assert')
    Buffer = require('buffer').Buffer;

if (process.setMaxListeners) process.setMaxListeners(0);

// lots of elmo
var elmo = fs.readFileSync(__dirname + '/support/elmo.png');

exports['blob test'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');
    var total = 10;
    var inserted = 0;
    var retrieved = 0;

    db.serialize(function() {
        db.run('CREATE TABLE elmos (id INT, image BLOB)');

        for (var i = 0; i < total; i++) {
            db.run('INSERT INTO elmos (id, image) VALUES (?, ?)', i, elmo, function(err) {
                if (err) throw err;
                inserted++;
            });
        }

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
        });

    });

    beforeExit(function() {
        assert.equal(inserted, total);
        assert.equal(retrieved, total);
    })
}
