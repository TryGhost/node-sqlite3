var sqlite3 = require('..');
var assert = require('assert');

describe('patching', function() {
    describe("Database", function() {
        var db;
        var originalFunctions = {};

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

        it('allow patching native functions', function() {
            var myFun = function myFunction() {
                return "Success";
            }
            
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.close = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.exec = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.wait = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.loadExtension = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.serialize = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.parallelize = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.configure = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Database.prototype.interrupt = myFun;
            });

            db = new sqlite3.Database(':memory:');
            assert.strictEqual(db.close(), "Success");
            assert.strictEqual(db.exec(), "Success");
            assert.strictEqual(db.wait(), "Success");
            assert.strictEqual(db.loadExtension(), "Success");
            assert.strictEqual(db.serialize(), "Success");
            assert.strictEqual(db.parallelize(), "Success");
            assert.strictEqual(db.configure(), "Success");
            assert.strictEqual(db.interrupt(), "Success");
        });

        after(function() {
            if(db != null) {
                sqlite3.Database.prototype.close = originalFunctions.close;
                sqlite3.Database.prototype.exec = originalFunctions.exec;
                sqlite3.Database.prototype.wait = originalFunctions.wait;
                sqlite3.Database.prototype.loadExtension = originalFunctions.loadExtension;
                sqlite3.Database.prototype.serialize = originalFunctions.serialize;
                sqlite3.Database.prototype.parallelize = originalFunctions.parallelize;
                sqlite3.Database.prototype.configure = originalFunctions.configure;
                sqlite3.Database.prototype.interrupt = originalFunctions.interrupt;
                db.close();
            }
        });
    });

    describe('Statement', function() {
        var db;
        var statement;
        var originalFunctions = {};

        before(function() {
            originalFunctions.bind = sqlite3.Statement.prototype.bind;
            originalFunctions.get = sqlite3.Statement.prototype.get;
            originalFunctions.run = sqlite3.Statement.prototype.run;
            originalFunctions.all = sqlite3.Statement.prototype.all;
            originalFunctions.each = sqlite3.Statement.prototype.each;
            originalFunctions.reset = sqlite3.Statement.prototype.reset;
            originalFunctions.finalize = sqlite3.Statement.prototype.finalize;
        });

        it('allow patching native functions', function() {
            var myFun = function myFunction() {
                return "Success";
            }
            
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.bind = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.get = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.run = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.all = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.each = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.reset = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Statement.prototype.finalize = myFun;
            });

            db = new sqlite3.Database(':memory:');
            statement = db.prepare("");
            assert.strictEqual(statement.bind(), "Success");
            assert.strictEqual(statement.get(), "Success");
            assert.strictEqual(statement.run(), "Success");
            assert.strictEqual(statement.all(), "Success");
            assert.strictEqual(statement.each(), "Success");
            assert.strictEqual(statement.reset(), "Success");
            assert.strictEqual(statement.finalize(), "Success");
        });

        after(function() {
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
                db.close();
            }
        });
    });

    describe('Backup', function() {
        var db;
        var backup;
        var originalFunctions = {};

        before(function() {
            originalFunctions.step = sqlite3.Backup.prototype.step;
            originalFunctions.finish = sqlite3.Backup.prototype.finish;
        });

        it('allow patching native functions', function() {
            var myFun = function myFunction() {
                return "Success";
            }
            
            assert.doesNotThrow(() => {
                sqlite3.Backup.prototype.step = myFun;
            });
            assert.doesNotThrow(() => {
                sqlite3.Backup.prototype.finish = myFun;
            });

            db = new sqlite3.Database(':memory:');
            backup = db.backup("somefile", myFun);
            assert.strictEqual(backup.step(), "Success");
            assert.strictEqual(backup.finish(), "Success");
        });

        after(function() {
            if(backup != null) {
                sqlite3.Backup.prototype.step = originalFunctions.step;
                sqlite3.Backup.prototype.finish = originalFunctions.finish;
                backup.finish();
            }
            if(db != null) {
                db.close();
            }
        });
    });
});
