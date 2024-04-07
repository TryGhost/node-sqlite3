var sqlite3 = require('..');

describe('fts', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
    });

    it('should create a new fts4 table', async function() {
        await db.exec('CREATE VIRTUAL TABLE t1 USING fts4(content="", a, b, c);');
    });
});
