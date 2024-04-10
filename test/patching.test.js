const sqlite3 = require('..');
const assert = require('assert');

describe('patching', function() {
    describe("Database", function() {
        /** @type {sqlite3.Database} */
        let db;
        const originalFunctions = {};

        before(function() {
            originalFunctions.close = sqlite3.Database.prototype.close;
            originalFunctions.exec = sqlite3.Database.prototype.exec;
            originalFunctions.wait = sqlite3.Database.prototype.wait;
            originalFunctions.loadExtension = sqlite3.Database.prototype.loadExtension;
            originalFunctions.serialize = sqlite3.Database.prototype.serialize;
            originalFunctions.parallelize = sqlite3.Database.prototype.parallelize;
            originalFunctions.configure = sqlite3.Database.prototype.configure;
            originalFunctions.interrupt = sqlite3.Database.prototype.interrupt;
        });

        it('allow patching native functions', async function() {
            async function myAsyncFn() {
                return "Success";
            }
            function myFn() {
                return "Success";
            }
            
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.close = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.exec = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.wait = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.loadExtension = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.serialize = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.parallelize = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.configure = myFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.interrupt = myFn;
            });

            db = await sqlite3.Database.create(':memory:');
            assert.strictEqual(await db.close(), "Success");
            assert.strictEqual(await db.exec(), "Success");
            assert.strictEqual(await db.wait(), "Success");
            assert.strictEqual(await db.loadExtension(), "Success");
            assert.strictEqual(await db.serialize(), "Success");
            assert.strictEqual(await db.parallelize(), "Success");
            assert.strictEqual(db.configure(), "Success");
            assert.strictEqual(db.interrupt(), "Success");
        });

        after(async function() {
            if(db != null) {
                sqlite3.Database.prototype.close = originalFunctions.close;
                sqlite3.Database.prototype.exec = originalFunctions.exec;
                sqlite3.Database.prototype.wait = originalFunctions.wait;
                sqlite3.Database.prototype.loadExtension = originalFunctions.loadExtension;
                sqlite3.Database.prototype.serialize = originalFunctions.serialize;
                sqlite3.Database.prototype.parallelize = originalFunctions.parallelize;
                sqlite3.Database.prototype.configure = originalFunctions.configure;
                sqlite3.Database.prototype.interrupt = originalFunctions.interrupt;
                await db.close();
            }
        });
    });

    describe('Statement', function() {
        /** @type {sqlite3.Database} */
        let db;
        /** @type {sqlite3.Statement} */
        let statement;
        const originalFunctions = {};

        before(function() {
            originalFunctions.bind = sqlite3.Statement.prototype.bind;
            originalFunctions.get = sqlite3.Statement.prototype.get;
            originalFunctions.run = sqlite3.Statement.prototype.run;
            originalFunctions.all = sqlite3.Statement.prototype.all;
            originalFunctions.each = sqlite3.Statement.prototype.each;
            originalFunctions.reset = sqlite3.Statement.prototype.reset;
            originalFunctions.finalize = sqlite3.Statement.prototype.finalize;
        });

        it('allow patching native functions', async function() {
            async function myAsyncFn() {
                return "Success";
            }
            async function* myGeneratorFn() {
                yield "Success";
            }

            
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.bind = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.get = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.run = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.all = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.each = myGeneratorFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.reset = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.finalize = myAsyncFn;
            });

            db = await sqlite3.Database.create(':memory:');
            statement = await db.prepare("");
            assert.strictEqual(await statement.bind(), "Success");
            assert.strictEqual(await statement.get(), "Success");
            assert.strictEqual(await statement.run(), "Success");
            assert.strictEqual(await statement.all(), "Success");
            assert.strictEqual((await statement.each().next()).value, "Success");
            assert.strictEqual(await statement.reset(), "Success");
            assert.strictEqual(await statement.finalize(), "Success");
        });

        after(async function() {
            if(statement != null) {
                sqlite3.Statement.prototype.bind = originalFunctions.bind;
                sqlite3.Statement.prototype.get = originalFunctions.get;
                sqlite3.Statement.prototype.run = originalFunctions.run;
                sqlite3.Statement.prototype.all = originalFunctions.all;
                sqlite3.Statement.prototype.each = originalFunctions.each;
                sqlite3.Statement.prototype.reset = originalFunctions.reset;
                sqlite3.Statement.prototype.finalize = originalFunctions.finalize;
            }
            if(db != null) {
                await db.close();
            }
        });
    });

    describe('Backup', function() {
        /** @type {sqlite3.Database} */
        let db;
        /** @type {sqlite3.Backup} */
        let backup;
        const originalFunctions = {};

        before(function() {
            originalFunctions.step = sqlite3.Backup.prototype.step;
            originalFunctions.finish = sqlite3.Backup.prototype.finish;
        });

        it('allow patching native functions', async function() {
            async function myAsyncFn() {
                return "Success";
            }
            
            assert.doesNotThrow(() => {
                sqlite3.Backup.prototype.step = myAsyncFn;
            });
            assert.doesNotThrow(() => {
                sqlite3.Backup.prototype.finish = myAsyncFn;
            });

            db = await sqlite3.Database.create(':memory:');
            backup = await db.backup("somefile");
            assert.strictEqual(await backup.step(), "Success");
            assert.strictEqual(await backup.finish(), "Success");
        });

        after(async function() {
            if(backup != null) {
                sqlite3.Backup.prototype.step = originalFunctions.step;
                sqlite3.Backup.prototype.finish = originalFunctions.finish;
                await backup.finish();
            }
            if(db != null) {
                await db.close();
            }
        });
    });
});
