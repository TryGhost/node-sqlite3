const sqlite3 = require('..');
const assert = require('assert');

describe('each', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create('test/support/big.db', sqlite3.OPEN_READONLY);
    });

    it('retrieve 100,000 rows with Statement#each', async function() {
        const total = 100000;
        // var total = 10;
        let retrieved = 0;
        
        const iterable = await db.each('SELECT id, txt FROM foo LIMIT 0, ?', total);
        for await (const _row of iterable) {
            retrieved++;
        }
        assert.equal(retrieved, total, "Only retrieved " + retrieved + " out of " + total + " rows.");
    });
 
    it('Statement#each with complete callback', async function() {
        const total = 10000;
        let retrieved = 0;

        for await (const _row of await db.each('SELECT id, txt FROM foo LIMIT 0, ?', total)) {
            retrieved++;
        }
        assert.equal(retrieved, total, "Only retrieved " + retrieved + " out of " + total + " rows.");
    });
});
