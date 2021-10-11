var sqlite3 = require('..');
var assert = require('assert');

describe('patching', function() {
    var db;
    var originalClose;

    it('allow patching native functions', function() {
        var myFun = function myFunction() {
            return "Success";
        }
        originalClose = sqlite3.Database.prototype.close;
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
            db.close = originalClose;
            db.close();
        }
    });

});
