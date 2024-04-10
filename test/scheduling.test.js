const sqlite3 = require('..');
const assert = require('assert');

describe('scheduling', function() {
    it('scheduling after the database was closed', async function() {
        const db = await sqlite3.Database.create(':memory:');

        await db.close();
        await assert.rejects(db.run("CREATE TABLE foo (id int)"), (err) => {
            assert.ok(err.message && err.message.indexOf("SQLITE_MISUSE: Database is closed") > -1);
            return true;
        });
    });

    it('running a query after the database was closed', async function() {
        const db = await sqlite3.Database.create(':memory:');

        const stmt = await db.prepare("SELECT * FROM sqlite_master");

        await assert.rejects(db.close(), (err) => {
            assert.ok(err);
            assert.ok(err.message && err.message.indexOf("SQLITE_BUSY: unable to close due to") > -1);
            return true;
        });
        // Running a statement now should not fail.
        await stmt.run();
        await stmt.finalize();
    });
});
