const sqlite3 = require('..');
const assert = require('node:assert');

describe('limit', function() {
    /** @type {sqlite3.Database} */
    let db;

    before(async function() {
        db = await sqlite3.Database.create(':memory:');
    });

    it('should support applying limits via configure', async function() {
        db.configure('limit', sqlite3.LIMIT_ATTACHED, 0);
        let caught;
        try {
            await db.exec("ATTACH 'test/support/prepare.db' AS zing");
        } catch (err) {
            caught = err;
        }

        if (!caught) {
            await db.close();
            throw new Error('ATTACH should not succeed');
        }

        assert.strictEqual(caught.errno, sqlite3.ERROR);
        assert.strictEqual(caught.message, 'SQLITE_ERROR: too many attached databases - max 0');
        db.configure('limit', sqlite3.LIMIT_ATTACHED, 1);
        await db.exec("ATTACH 'test/support/prepare.db' AS zing");
        await db.close();
    });
});
