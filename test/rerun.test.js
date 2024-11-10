const sqlite3 = require('..');
const assert = require('assert');

describe('rerunning statements', function() {
    /** @type {sqlite3.Database} */
    let db;
    const count = 10;

    before(async function() {
        db = await sqlite3.Database.create(':memory:');
        await db.run("CREATE TABLE foo (id int)");
        
        const stmt = await db.prepare("INSERT INTO foo VALUES(?)");
        for (let i = 5; i < count; i++) {
            await stmt.run(i);
        }
    
        await stmt.finalize();
    });

    it('should retrieve repeatedly, resuing the same statement', async function() {
        const collected = [];
        const stmt = await db.prepare("SELECT id FROM foo WHERE id = ?");
        for (let i = 0; i < count; i++) {
            const row = await stmt.get(i);
            if (row) collected.push(row);
        }
        await stmt.finalize();
        assert.deepEqual(collected, [ { id: 5 }, { id: 6 }, { id: 7 }, { id: 8 }, { id: 9 } ]);
        assert.equal(collected.length, 5);
    });

    after(async function() { 
        await db.close();
    });
});
