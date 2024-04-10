const sqlite3 = require('..'),
    fs = require('fs'),
    assert = require('assert'),
    Buffer = require('buffer').Buffer;

// lots of elmo
const elmo = fs.readFileSync(__dirname + '/support/elmo.png');

describe('blob', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
        await db.run("CREATE TABLE elmos (id INT, image BLOB)");
    });

    const total = 10;
    let inserted = 0;
    let retrieved = 0;

    it('should insert blobs', async function() {
        const promises = [];
        for (let i = 0; i < total; i++) {
            promises.push(
                db.run('INSERT INTO elmos (id, image) VALUES (?, ?)', i, elmo).then(() => {
                    inserted++;
                })
            );
        }
        // TODO: fix wait
        // await db.wait();
        await Promise.all(promises);
        assert.equal(inserted, total);
    });

    it('should retrieve the blobs', async function() {
        const rows = await db.all('SELECT id, image FROM elmos ORDER BY id');
        for (let i = 0; i < rows.length; i++) {
            assert.ok(Buffer.isBuffer(rows[i].image));
            assert.ok(elmo.length, rows[i].image);

            for (let j = 0; j < elmo.length; j++) {
                if (elmo[j] !== rows[i].image[j]) {
                    assert.ok(false, "Wrong byte");
                }
            }

            retrieved++;
        }

        assert.equal(retrieved, total);
    });
});
