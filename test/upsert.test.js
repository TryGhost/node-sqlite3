const sqlite3 = require('..');
const assert = require('assert');

describe('query properties', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
        await db.run("CREATE TABLE foo (id INT PRIMARY KEY, count INT)");
    });

    (sqlite3.VERSION_NUMBER < 3024000 ? it.skip : it)('should upsert', async function() {
        const stmt = await db.prepare("INSERT INTO foo VALUES(?, ?)");
        await stmt.run(1, 1);
        await stmt.finalize();

        const upsertStmt = await db.prepare("INSERT INTO foo VALUES(?, ?) ON CONFLICT (id) DO UPDATE SET count = count + excluded.count");
        await upsertStmt.run(1, 2);
        await upsertStmt.finalize();

        const selectStmt = await db.prepare("SELECT count FROM foo WHERE id = ?");
        const row = await selectStmt.get(1);
        await selectStmt.finalize();
        assert.equal(row.count, 3);
    });
});
