const sqlite3 = require('..');
const assert = require('assert');

describe('prepare', function() {
    describe('invalid SQL', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create(':memory:');
        });

        it('should fail preparing a statement with invalid SQL', async function() {
            await assert.rejects(db.prepare('CRATE TALE foo text bar)'), (err) => {
                assert.strictEqual(err.errno, sqlite3.ERROR);
                assert.strictEqual(err.message, 'SQLITE_ERROR: near "CRATE": syntax error');
                return true;
            });
        });

        after(async function() {
            await db.close();
        });
    });

    describe('simple prepared statement', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create(':memory:');
        });

        it('should prepare, run and finalize the statement', async function() {
            await db.prepare("CREATE TABLE foo (text bar)")
                .then((statement) => statement.run())
                .then((statement) => statement.finalize());
        });

        after(async function() {
            db.close();
        });
    });

    describe('inserting and retrieving rows', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create(':memory:');
        });

        let inserted = 0;
        let retrieved = 0;

        // We insert and retrieve that many rows.
        const count = 1000;
        
        it('should prepare a statement and run it ' + (count + 5) + ' times', async function() {
            const statement = await db.prepare("CREATE TABLE foo (txt text, num int, flt float, blb blob)");
            await statement.run();
            await statement.finalize();

            for (let i = 0; i < count; i++) {
                const statement = await db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
                await statement.run('String ' + i, i, i * Math.PI);
                inserted++;
                await statement.finalize();
            }
            assert.strictEqual(inserted, count);
            const stmt = await db.prepare("SELECT txt, num, flt, blb FROM foo ORDER BY num");
            assert.equal(stmt.sql, 'SELECT txt, num, flt, blb FROM foo ORDER BY num');

            for (let i = 0; i < count; i++)  {
                const row = await stmt.get();
                assert.equal(row.txt, 'String ' + i);
                assert.equal(row.num, i);
                assert.equal(row.flt, i * Math.PI);
                assert.equal(row.blb, null);

                retrieved++;
            }
            
            for (let i = count; i < count + 5; i++)  {
                const row = await stmt.get();
                assert.equal(row, undefined);

                retrieved++;
            }

            await stmt.finalize();
            assert.equal(count + 5, retrieved, "Didn't retrieve all rows");
        });


        after(async function() {
            db.close();
        });
    });

    describe('inserting with accidental undefined', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create(':memory:');
        });

        let retrieved = 0;
        
        it('should retrieve the data', async function() {
            await db.prepare("CREATE TABLE foo (num int)")
                .then((statement) => statement.run())
                .then((statement) => statement.finalize());

            await db.prepare('INSERT INTO foo VALUES(4)')
                .then((statement) => statement.run())
                // The second time we pass undefined as a parameter. This is
                // a mistake, but it should either throw an error or be ignored,
                // not silently fail to run the statement.
                .then((statement) => statement.run(undefined))
                .then((statement) => statement.finalize());

            const stmt = await db.prepare("SELECT num FROM foo");

            for (let i = 0; i < 2; i++) {
                const row = await stmt.get();
                assert(row);
                assert.equal(row.num, 4);
                retrieved++;
            }

            await stmt.finalize();
            assert.equal(2, retrieved, "Didn't retrieve all rows");
        });

        after(async function() {
            await db.close();
        });
    });

    describe('retrieving reset() function', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create('test/support/prepare.db', sqlite3.OPEN_READONLY);
        });

        let retrieved = 0;

        it('should retrieve the same row over and over again', async function() {
            const stmt = await db.prepare("SELECT txt, num, flt, blb FROM foo ORDER BY num");
            for (let i = 0; i < 10; i++) {
                await stmt.reset();
                const row = await stmt.get();
                assert.equal(row.txt, 'String 0');
                assert.equal(row.num, 0);
                assert.equal(row.flt, 0.0);
                assert.equal(row.blb, null);
                retrieved++;
            }
            await stmt.finalize();
            assert.equal(10, retrieved, "Didn't retrieve all rows");
        });

        after(async function() {
            await db.close();
        });
    });

    describe('multiple get() parameter binding', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create('test/support/prepare.db', sqlite3.OPEN_READONLY);
        });

        let retrieved = 0;

        it('should retrieve particular rows', async function() {
            const stmt = await db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num = ?");

            for (let i = 0; i < 10; i++) {
                const row = await stmt.get(i * 10 + 1);
                const val = i * 10 + 1;
                assert.equal(row.txt, 'String ' + val);
                assert.equal(row.num, val);
                assert.equal(row.flt, val * Math.PI);
                assert.equal(row.blb, null);
                retrieved++;
            }

            await stmt.finalize();
            assert.equal(10, retrieved, "Didn't retrieve all rows");
        });

        after(async function() {
            await db.close();
        });
    });

    describe('prepare() parameter binding', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create('test/support/prepare.db', sqlite3.OPEN_READONLY);
        });

        it('should retrieve particular rows', async function() {
            const statement = await db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num = ? AND txt = ?", 10, 'String 10');
            const row = await statement.get();
            await statement.finalize();
            assert.equal(row.txt, 'String 10');
            assert.equal(row.num, 10);
            assert.equal(row.flt, 10 * Math.PI);
            assert.equal(row.blb, null);
        });

        after(async function() {
            await db.close();
        });
    });

    describe('all()', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create('test/support/prepare.db', sqlite3.OPEN_READONLY);
        });

        let retrieved = 0;
        const count = 1000;

        it('should retrieve particular rows', async function() {
            const statement = await db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num < ? ORDER BY num", count);
            const rows = await statement.all();
            await statement.finalize();
            for (let i = 0; i < rows.length; i++) {
                assert.equal(rows[i].txt, 'String ' + i);
                assert.equal(rows[i].num, i);
                assert.equal(rows[i].flt, i * Math.PI);
                assert.equal(rows[i].blb, null);
                retrieved++;
            }
            assert.equal(count, retrieved, "Didn't retrieve all rows");
        });

        after(async function() {
            await db.close();
        });
    });

    describe('all()', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create('test/support/prepare.db', sqlite3.OPEN_READONLY);
        });

        it('should retrieve particular rows', async function() {
            const statement = await db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num > 5000");
            const rows = await statement.all();
            await statement.finalize();
            assert.ok(rows.length === 0);
        });

        after(async function() { 
            await db.close();
        });
    });

    describe('high concurrency', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create(':memory:');
        });

        function randomString() {
            let str = '';
            for (let i = Math.random() * 300; i > 0; i--) {
                str += String.fromCharCode(Math.floor(Math.random() * 256));
            }
            return str;
        }

        // Generate random data.
        const data = [];
        const length = Math.floor(Math.random() * 1000) + 200;
        for (let i = 0; i < length; i++) {
            data.push([ randomString(), i, i * Math.random(), null ]);
        }

        let inserted = 0;
        let retrieved = 0;
        
        it('should retrieve all values', async function() {
            await db.prepare("CREATE TABLE foo (txt text, num int, flt float, blb blob)")
                .then((statement) => statement.run())
                .then((statement) => statement.finalize());

            for (let i = 0; i < data.length; i++) {
                const stmt = await db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
                await stmt.run(data[i][0], data[i][1], data[i][2], data[i][3]);
                await stmt.finalize();
                inserted++;
            }

            const statement = await db.prepare("SELECT txt, num, flt, blb FROM foo");
            const rows = await statement.all();
            await statement.finalize();

            for (let i = 0; i < rows.length; i++) {
                assert.ok(data[rows[i].num] !== true);

                assert.equal(rows[i].txt, data[rows[i].num][0]);
                assert.equal(rows[i].num, data[rows[i].num][1]);
                assert.equal(rows[i].flt, data[rows[i].num][2]);
                assert.equal(rows[i].blb, data[rows[i].num][3]);

                // Mark the data row as already retrieved.
                data[rows[i].num] = true;
                retrieved++;
            }

            assert.equal(retrieved, data.length);
            assert.equal(retrieved, inserted);
        });

        after(async function() {
            await db.close();
        });
    });


    describe('test Database#get()', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create('test/support/prepare.db', sqlite3.OPEN_READONLY);
        });

        let retrieved = 0;

        it('should get a row', async function() {
            const row = await db.get("SELECT txt, num, flt, blb FROM foo WHERE num = ? AND txt = ?", 10, 'String 10');
            assert.equal(row.txt, 'String 10');
            assert.equal(row.num, 10);
            assert.equal(row.flt, 10 * Math.PI);
            assert.equal(row.blb, null);
            retrieved++;
            assert.equal(1, retrieved, "Didn't retrieve all rows");
        });

        after(async function() { 
            await db.close();
        });
    });

    describe('Database#run() and Database#all()', function() {
        /** @type {sqlite3.Database} */
        let db;
        before(async function() {
            db = await sqlite3.Database.create(':memory:');
        });

        let inserted = 0;
        let retrieved = 0;

        // We insert and retrieve that many rows.
        const count = 1000;
        
        it('should retrieve all rows', async function() {
            await db.run("CREATE TABLE foo (txt text, num int, flt float, blb blob)");
    
            for (let i = 0; i < count; i++) {
                await db.run("INSERT INTO foo VALUES(?, ?, ?, ?)", 'String ' + i, i, i * Math.PI);
                inserted++;
            }

            const rows = await db.all("SELECT txt, num, flt, blb FROM foo ORDER BY num");

            for (let i = 0; i < rows.length; i++) {
                assert.equal(rows[i].txt, 'String ' + i);
                assert.equal(rows[i].num, i);
                assert.equal(rows[i].flt, i * Math.PI);
                assert.equal(rows[i].blb, null);
                retrieved++;
            }

            assert.equal(retrieved, count);
            assert.equal(retrieved, inserted);
        });

        after(async function() {
            await db.close();
        });
    });
});
