const sqlite3 = require('..');
const assert = require('assert');

describe('error handling', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
    });

    it('throw when calling Database() without new', function() {
        assert.throws(function() {
            // @ts-expect-error Calling private constructor for test
            sqlite3.Database(':memory:');
        }, (/Class constructors cannot be invoked without 'new'/));

        assert.throws(function() {
            // @ts-expect-error Calling private constructor for test
            sqlite3.Statement();
        }, (/Class constructors cannot be invoked without 'new'/));
    });

    it('should error when calling Database#get on a missing table', async function() {
        try {
            await db.get('SELECT id, txt FROM foo');
        } catch (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
        }
        new Error('Completed query without error, but expected error');
    });

    it('Database#all prepare fail', async function() {
        try {
            await db.all('SELECT id, txt FROM foo');
        } catch (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
        }
        new Error('Completed query without error, but expected error');
    });

    it('Database#run prepare fail', async function() {
        try {

            await db.run('SELECT id, txt FROM foo');
        } catch (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
        }
        new Error('Completed query without error, but expected error');
    });

    it('Database#each prepare fail', async function() {
        await assert.rejects(db.each('SELECT id, text FROM foo'), (err) => {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            return true;
        });
    });

    it('Database#each prepare fail without completion handler', async function() {
        await assert.rejects(db.each('SELECT id, txt FROM foo'), (err) => {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            return true;
        });
    });

    it('Database#get prepare fail with param binding', async function() {
        try {
            await db.get('SELECT id, txt FROM foo WHERE id = ?', 1);
        } catch (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
        }
        new Error('Completed query without error, but expected error');
    });

    it('Database#all prepare fail with param binding', async function() {
        try {
            await db.all('SELECT id, txt FROM foo WHERE id = ?', 1);
        } catch (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
        }
        new Error('Completed query without error, but expected error');
    });

    it('Database#run prepare fail with param binding', async function() {
        try {
            await db.run('SELECT id, txt FROM foo WHERE id = ?', 1);
        } catch (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
        }
        new Error('Completed query without error, but expected error');
    });

    it('Database#each prepare fail with param binding', async function() {
        await assert.rejects(db.each('SELECT id, txt FROM foo WHERE id = ?', 1), (err) => {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            return true;
        });
    });

    it('Database#each prepare fail with param binding without completion handler', async function() {
        assert.rejects(db.each('SELECT id, txt FROM foo WHERE id = ?', 1), (err) => {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            return true;
        });
    });

    after(async function() {
        await db.close();
    });
});
