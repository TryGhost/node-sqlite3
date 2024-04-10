const sqlite3 = require('..');
const assert = require('assert');

describe('map', function() {
    it('test Database#map() with two columns', async function() {
        const count = 10;
        let inserted = 0;

        const db = await sqlite3.Database.create(':memory:');
        await db.serialize(async function() {
            await db.run("CREATE TABLE foo (id INT, value TEXT)");

            const stmt = await db.prepare("INSERT INTO foo VALUES(?, ?)");
            for (let i = 5; i < count; i++) {
                await stmt.run(i, 'Value for ' + i);
                inserted++;
            }
            await stmt.finalize();

            const results = await db.map("SELECT * FROM foo");
            assert.deepEqual(results, { 5: 'Value for 5', 6: 'Value for 6', 7: 'Value for 7', 8: 'Value for 8', 9: 'Value for 9' });
            assert.equal(inserted, 5);
        });
    });

    it('test Database#map() with three columns', async function() {
        const db = await sqlite3.Database.create(':memory:');

        const count = 10;
        let inserted = 0;

        await db.serialize(async function() {
            await db.run("CREATE TABLE foo (id INT, value TEXT, other TEXT)");

            const stmt = await db.prepare("INSERT INTO foo VALUES(?, ?, ?)");
            for (let i = 5; i < count; i++) {
                await stmt.run(i, 'Value for ' + i);
                inserted++;
            }
            await stmt.finalize();

            const results = await db.map("SELECT * FROM foo");
            assert.deepEqual(results, {
                5: { id: 5, value: 'Value for 5', other: null },
                6: { id: 6, value: 'Value for 6', other: null },
                7: { id: 7, value: 'Value for 7', other: null },
                8: { id: 8, value: 'Value for 8', other: null },
                9: { id: 9, value: 'Value for 9', other: null }
            });
            assert.equal(inserted, 5);
        });
    });
});
