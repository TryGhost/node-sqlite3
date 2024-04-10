const sqlite3 = require('..');
const assert = require('assert');

describe('named columns', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
    });

    it('should create the table', async function() {
        await db.run("CREATE TABLE foo (txt TEXT, num INT)");
    });

    it('should insert a value', async function() {
        await db.run("INSERT INTO foo VALUES($text, $id)", {
            $id: 1,
            $text: "Lorem Ipsum"
        });
    });

    it('should retrieve the values', async function() {
        const row = await db.get("SELECT txt, num FROM foo ORDER BY num");
        assert.equal(row.txt, "Lorem Ipsum");
        assert.equal(row.num, 1);
    });

    it('should be able to retrieve rowid of last inserted value', async function() {
        const row = await db.get("SELECT last_insert_rowid() as last_id FROM foo");
        assert.equal(row.last_id, 1);
    });

    after(async function() {
        await db.close();
    });
});
