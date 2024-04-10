const sqlite3 = require('..');
const assert = require('assert');
const helper = require('./support/helper');

describe('open/close', function() {
    before(function() {
        helper.ensureExists('test/tmp');
    });

    describe('open and close non-existant database', async function() {
        before(function() {
            helper.deleteFile('test/tmp/test_create.db');
        });

        /** @type {sqlite3.Database} */
        let db;
        it('should open the database', async function() {
            db = await sqlite3.Database.create('test/tmp/test_create.db');
        });

        it('should close the database', async function() {
            await db.close();
        });

        it('should have created the file', function() {
            assert.fileExists('test/tmp/test_create.db');
        });

        after(function() {
            helper.deleteFile('test/tmp/test_create.db');
        });
    });
    
    describe('open and close non-existant shared database', function() {
        before(function() {
            helper.deleteFile('test/tmp/test_create_shared.db');
        });

        /** @type {sqlite3.Database} */
        let db;
        it('should open the database', async function() {
            db = await sqlite3.Database.create('file:./test/tmp/test_create_shared.db', sqlite3.OPEN_URI | sqlite3.OPEN_SHAREDCACHE | sqlite3.OPEN_READWRITE | sqlite3.OPEN_CREATE);
        });

        it('should close the database', async function() {
            await db.close();
        });

        it('should have created the file', function() {
            assert.fileExists('test/tmp/test_create_shared.db');
        });

        after(function() {
            helper.deleteFile('test/tmp/test_create_shared.db');
        });
    });


    (sqlite3.VERSION_NUMBER < 3008000 ? describe.skip : describe)('open and close shared memory database', function() {

        /** @type {sqlite3.Database} */
        let db1;
        /** @type {sqlite3.Database} */
        let db2;

        it('should open the first database', async function() {
            db1 = await sqlite3.Database.create('file:./test/tmp/test_memory.db?mode=memory', sqlite3.OPEN_URI | sqlite3.OPEN_SHAREDCACHE | sqlite3.OPEN_READWRITE | sqlite3.OPEN_CREATE);
        });

        it('should open the second database', async function() {
            db2 = await sqlite3.Database.create('file:./test/tmp/test_memory.db?mode=memory', sqlite3.OPEN_URI | sqlite3.OPEN_SHAREDCACHE | sqlite3.OPEN_READWRITE | sqlite3.OPEN_CREATE);
        });

        it('first database should set the user_version', async function() {
            await db1.exec('PRAGMA user_version=42');
        });

        it('second database should get the user_version', async function() {
            const row = await db2.get('PRAGMA user_version');
            assert.equal(row.user_version, 42);
        });

        it('should close the first database', async function() {
            await db1.close();
        });

        it('should close the second database', async function() {
            await db2.close();
        });
    });

    it('should not be unable to open an inaccessible database', async function() {
        await assert.rejects(async () => {
            const db = await sqlite3.Database.create('/test/tmp/directory-does-not-exist/test.db');
            await db.close();
        }, (err) => {
            assert.strictEqual(err.errno, sqlite3.CANTOPEN);
            return true;
        });
    });


    describe('creating database without create flag', function() {
        before(function() {
            helper.deleteFile('test/tmp/test_readonly.db');
        });

        it('should fail to open the database', async function() {
            await assert.rejects(async () => {
                const db = await sqlite3.Database.create('test/tmp/test_readonly.db', sqlite3.READONLY);
                await db.close();
            }, (err) => {
                // TODO: This is the wrong error code?
                assert.strictEqual(err.errno, sqlite3.MISUSE);
                return true;
            });
            assert.fileDoesNotExist('test/tmp/test_readonly.db');
        });

        after(function() {
            helper.deleteFile('test/tmp/test_readonly.db');
        });
    });

    describe('open and close memory database queuing', function() {

        
        it('shouldn\'t close the database twice', async function() {
            const db = await sqlite3.Database.create(':memory:');
            await db.close();
            await assert.rejects(db.close(), (err) => {
                assert.ok(err, 'No error object received on second close');
                assert.ok(err.errno === sqlite3.MISUSE);
                return true;
            });
        });
    });

    describe('closing with unfinalized statements', function(done) {
        /** @type {sqlite3.Database} */
        let db;
        /** @type {sqlite3.Statement} */
        let stmt;
        before(async function() {
            db = await sqlite3.Database.create(':memory:', done);
            await db.run("CREATE TABLE foo (id INT, num INT)");
            stmt = await db.prepare('INSERT INTO foo VALUES (?, ?)');
            stmt.run(1, 2, done);
        });

        it('should fail to close the database', async function() {
            await assert.rejects(db.close(), function(err) {
                assert.ok(err.message,
                    "SQLITE_BUSY: unable to close due to unfinalised statements");
                return true;
            });
        });

        it('should succeed to close the database after finalizing', async function() {
            await stmt.run(3, 4);
            await stmt.finalize();
            await db.close();
        });
    });
});
