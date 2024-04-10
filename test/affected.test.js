const sqlite3 = require('..');
const assert = require('assert');

describe('query properties', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
        await db.run("CREATE TABLE foo (id INT, txt TEXT)");
    });

    it('should return the correct lastID', async function() {
        const stmt = await db.prepare("INSERT INTO foo VALUES(?, ?)");
        let j = 1;
        const promises = [];
        for (let i = 0; i < 5000; i++) {
            promises.push(stmt.run(i, "demo").then((result) => {
                // Relies on SQLite's row numbering to be gapless and starting
                // from 1.
                assert.equal(j++, result.lastID);
            }));
        }
        await Promise.all(promises);
        // TODO: this doesn't seem to work?
        // await db.wait();
    });

    it('should return the correct changes count', async function() {
        const statement = await db.run("UPDATE foo SET id = id + 1 WHERE id % 2 = 0");
        await db.all("SELECT * FROM foo");
        assert.equal(2500, statement.changes);
    });
});
