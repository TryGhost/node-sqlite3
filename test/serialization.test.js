const sqlite3 = require('..');
const assert = require('assert');


describe('serialize() and parallelize()', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
    });

    let inserted1 = 0;
    let inserted2 = 0;
    let retrieved = 0;

    const count = 1000;

    it('should toggle', async function() {
        await db.serialize();
        await db.run("CREATE TABLE foo (txt text, num int, flt float, blb blob)");
        await db.parallelize();
    });

    it('should insert rows', async function() {
        const stmt1 = await db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
        const stmt2 = await db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
        for (let i = 0; i < count; i++) {
            // Interleaved inserts with two statements.
            await stmt1.run('String ' + i, i, i * Math.PI);
            inserted1++;
            i++;
            await stmt2.run('String ' + i, i, i * Math.PI);
            inserted2++;
        }
        await stmt1.finalize();
        await stmt2.finalize();
    });

    it('should have inserted all the rows after synchronizing with serialize()', async function() {
        await db.serialize();
        const rows = await db.all("SELECT txt, num, flt, blb FROM foo ORDER BY num");

        for (let i = 0; i < rows.length; i++) {
            assert.equal(rows[i].txt, 'String ' + i);
            assert.equal(rows[i].num, i);
            assert.equal(rows[i].flt, i * Math.PI);
            assert.equal(rows[i].blb, null);
            retrieved++;
        }

        assert.equal(count, inserted1 + inserted2, "Didn't insert all rows");
        assert.equal(count, retrieved, "Didn't retrieve all rows");
    });

    after(async function() {
        await db.close();
    });
});

describe('serialize(fn)', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
    });

    let inserted = 0;
    let retrieved = 0;

    const count = 1000;

    it('should call the callback', async function() {
        await db.serialize(async function() {
            await db.run("CREATE TABLE foo (txt text, num int, flt float, blb blob)");

            const stmt = await db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
            for (let i = 0; i < count; i++) {
                await stmt.run('String ' + i, i, i * Math.PI);
                inserted++;
            }
            await stmt.finalize();

            const rows = await db.all("SELECT txt, num, flt, blb FROM foo ORDER BY num");

            for (let i = 0; i < rows.length; i++) {
                assert.equal(rows[i].txt, 'String ' + i);
                assert.equal(rows[i].num, i);
                assert.equal(rows[i].flt, i * Math.PI);
                assert.equal(rows[i].blb, null);
                retrieved++;
            }
        });

        assert.equal(count, inserted, "Didn't insert all rows");
        assert.equal(count, retrieved, "Didn't retrieve all rows");
    });

    after(async function() {
        await db.close();
    });
});
