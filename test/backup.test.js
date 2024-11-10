const sqlite3 = require('..');
const assert = require('assert');
const helper = require('./support/helper');

// Check that the number of rows in two tables matches.
async function assertRowsMatchDb(db1, table1, db2, table2) {
    const row = await db1.get("SELECT COUNT(*) as count FROM " + table1);
    const row2 = await db2.get("SELECT COUNT(*) as count FROM " + table2);
    assert.equal(row.count, row2.count);
}

// Check that the number of rows in the table "foo" is preserved in a backup.
async function assertRowsMatchFile(db, backupName) {
    const db2 = await sqlite3.Database.create(backupName, sqlite3.OPEN_READONLY);
    await assertRowsMatchDb(db, 'foo', db2, 'foo');
    await db2.close();
}

describe('backup', function() {
    before(function() {
        helper.ensureExists('test/tmp');
    });

    /** @type {sqlite3.Database} */
    let db;
    beforeEach(async function() {
        helper.deleteFile('test/tmp/backup.db');
        helper.deleteFile('test/tmp/backup2.db');
        db = await sqlite3.Database.create('test/support/prepare.db', sqlite3.OPEN_READONLY);
    });

    afterEach(async function() {
        await (db && db.close());
    });

    it('output db created once step is called', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        await backup.step(1);
        assert.fileExists('test/tmp/backup.db');
        await backup.finish();
    });

    it('copies source fully with step(-1)', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        await backup.step(-1);
        assert.fileExists('test/tmp/backup.db');
        await backup.finish();
        await assertRowsMatchFile(db, 'test/tmp/backup.db');
    });

    it('backup db not created if finished immediately', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        await backup.finish();
        assert.fileDoesNotExist('test/tmp/backup.db');
    });

    it('error closing db if backup not finished', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        try {
            await db.close();
        } catch (err) {
            assert.equal(err.errno, sqlite3.BUSY);
        } finally {
            // Finish the backup so that the after hook succeeds
            await backup.finish();
        }
    });

    it('using the backup after finished is an error', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        await backup.finish();
        try {
            await backup.step(1);
            
        } catch (err) {
            assert.equal(err.errno, sqlite3.MISUSE);
            assert.equal(err.message, 'SQLITE_MISUSE: Backup is already finished');
        }
    });

    it('remaining/pageCount are available after call to step', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        await backup.step(0);
        assert.equal(typeof backup.pageCount, 'number');
        assert.equal(typeof backup.remaining, 'number');
        assert.equal(backup.remaining, backup.pageCount);
        const prevRemaining = backup.remaining;
        const prevPageCount = backup.pageCount;
        await backup.step(1);
        assert.notEqual(backup.remaining, prevRemaining);
        assert.equal(backup.pageCount, prevPageCount);
        await backup.finish();
    });

    it('backup works if database is modified half-way through', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        await backup.step(-1);
        await backup.finish();
        const db2 = await sqlite3.Database.create('test/tmp/backup.db');
        const backup2 = await db2.backup('test/tmp/backup2.db');
        const completed = await backup2.step(1);
        assert.equal(completed, false);  // Page size for the test db
        // should not be raised to high.
        await db2.exec("insert into foo(txt) values('hello')");
        const completed2 = await backup2.step(-1);
        assert.equal(completed2, true);
        await assertRowsMatchFile(db2, 'test/tmp/backup2.db');
        await backup2.finish();
        await db2.close();
    });

    (sqlite3.VERSION_NUMBER < 3026000 ? it.skip : it) ('can backup from temp to main', async function() {
        await db.exec("CREATE TEMP TABLE space (txt TEXT)");
        await db.exec("INSERT INTO space(txt) VALUES('monkey')");
        const backup = await db.backup('test/tmp/backup.db', 'temp', 'main', true);
        await backup.step(-1);
        await backup.finish();
        const db2 = await sqlite3.Database.create('test/tmp/backup.db');
        const row = await db2.get("SELECT * FROM space");
        assert.equal(row.txt, 'monkey');
        await db2.close();
    });

    (sqlite3.VERSION_NUMBER < 3026000 ? it.skip : it) ('can backup from main to temp', async function() {
        const backup = await db.backup('test/support/prepare.db', 'main', 'temp', false);
        await backup.step(-1);
        await backup.finish();
        await assertRowsMatchDb(db, 'temp.foo', db, 'main.foo');
    });

    it('cannot backup to a locked db', async function() {
        const db2 = await sqlite3.Database.create('test/tmp/backup.db');
        await db2.exec("PRAGMA locking_mode = EXCLUSIVE");
        await db2.exec("BEGIN EXCLUSIVE");
        const backup = await db.backup('test/tmp/backup.db');
        try {
            await backup.step(-1);
        } catch (err) {
            assert.equal(err.errno, sqlite3.BUSY);
        } finally {
            await backup.finish();
        }
    });

    it('fuss-free incremental backups work', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        let timer;
        let resolve;
        const promise = new Promise((res) => {
            resolve = res;
        });
        async function makeProgress() {
            if (backup.idle) {
                await backup.step(1);
            }
            if (backup.completed || backup.failed) {
                clearInterval(timer);
                assert.equal(backup.completed, true);
                assert.equal(backup.failed, false);
                resolve();
            }
        }
        timer = setInterval(makeProgress, 2);
        await promise;
    });

    it('setting retryErrors to empty disables automatic finishing', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        backup.retryErrors = [];
        await backup.step(-1);
        try {
            await db.close();
        } catch (err) {
            assert.equal(err.errno, sqlite3.BUSY);
        } finally {
            await backup.finish();
        }
    });

    it('setting retryErrors enables automatic finishing', async function() {
        const backup = await db.backup('test/tmp/backup.db');
        backup.retryErrors = [sqlite3.OK];
        await backup.step(-1);
    });

    it('default retryErrors will retry on a locked/busy db', async function() {
        const db2 = await sqlite3.Database.create('test/tmp/backup.db');
        await db2.exec("PRAGMA locking_mode = EXCLUSIVE");
        await db2.exec("BEGIN EXCLUSIVE");
        const backup = await db.backup('test/tmp/backup.db');
        try {
            await backup.step(-1);
        } catch (err) {
            assert.equal(err.errno, sqlite3.BUSY);
        }
        await db2.close();
        assert.equal(backup.completed, false);
        assert.equal(backup.failed, false);
        await backup.step(-1);
        assert.equal(backup.completed, true);
        assert.equal(backup.failed, false);
    });
});
