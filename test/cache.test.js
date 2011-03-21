var sqlite3 = require('sqlite3');
var assert = require('assert');
var helper = require('./support/helper');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test caching Database objects while opening'] = function(beforeExit) {
    var filename = 'test/tmp/test_cache.db';
    helper.deleteFile(filename);
    var opened1 = false, opened2 = false
    var db1 = new sqlite3.cached.Database(filename, function(err) {
        if (err) throw err;
        opened1 = true;
    });
    var db2 = new sqlite3.cached.Database(filename, function(err) {
        if (err) throw err;
        opened2 = true;
    });
    assert.equal(db1, db2);

    beforeExit(function() {
        assert.ok(opened1);
        assert.ok(opened2);
    });
};

exports['test caching Database objects after it is open'] = function(beforeExit) {
    var filename = 'test/tmp/test_cache2.db';
    helper.deleteFile(filename);
    var opened1 = false, opened2 = false
    var db1, db2;
    db1 = new sqlite3.cached.Database(filename, function(err) {
        if (err) throw err;
        opened1 = true;
        setTimeout(function() {
            db2 = new sqlite3.cached.Database(filename, function(err) {
                opened2 = true;
            });
        }, 100);
    });
    

    beforeExit(function() {
        assert.equal(db1, db2);
        assert.ok(opened1);
        assert.ok(opened2);
    });
};
