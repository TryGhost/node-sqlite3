var sqlite3 = require('..');
var assert = require('assert');

describe('patching', function() {
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
