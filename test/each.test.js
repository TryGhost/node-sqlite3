var sqlite3 = require('..');
var assert = require('assert');

describe('each', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create('test/support/big.db', sqlite3.OPEN_READONLY);
    });

    it('retrieve 100,000 rows with Statement#each', async function() {
        var total = 100000;
        // var total = 10;
        var retrieved = 0;
        
        const iterable = await db.each('SELECT id, txt FROM foo LIMIT 0, ?', total)
        for await (const row of iterable) {
            retrieved++
        }
        assert.equal(retrieved, total, "Only retrieved " + retrieved + " out of " + total + " rows.");
    });

    it.skip('Statement#each with complete callback', function(done) {
        var total = 10000;
        var retrieved = 0;

        db.each('SELECT id, txt FROM foo LIMIT 0, ?', total, function(err, row) {
            if (err) throw err;
            retrieved++;
        }, function(err, num) {
            assert.equal(retrieved, num);
            assert.equal(retrieved, total, "Only retrieved " + retrieved + " out of " + total + " rows.");
            done();
        });
    });
});
