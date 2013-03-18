var sqlite3 = require('sqlite3');
var assert = require('assert');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test scheduling a query after the database was closed'] = function(beforeExit) {
    var error = false;
    var db = new sqlite3.Database(':memory:');
    db.on('error', function(err) {
        error = true;
        assert.ok(err.message && err.message.indexOf("SQLITE_MISUSE: Database handle is closed") > -1);
    });

    db.close();
    db.run("CREATE TABLE foo (id int)");

    beforeExit(function() {
        assert.ok(error);
    });
};


exports['test scheduling a query with callback after the database was closed'] = function(beforeExit) {
    var error = false;
    var errorEvent = false;
    var db = new sqlite3.Database(':memory:');
    db.on('error', function(err) {
        errorEvent = true;
    });

    db.close();
    db.run("CREATE TABLE foo (id int)", function(err) {
        assert.ok(err.message && err.message.indexOf("SQLITE_MISUSE: Database handle is closed") > -1);
        error = true;
    });

    beforeExit(function() {
        assert.ok(error);
        assert.ok(!errorEvent);
    });
};

exports['test running a query after the database was closed'] = function(beforeExit) {
    var error = false;
    var db = new sqlite3.Database(':memory:');

    var stmt = db.prepare("SELECT * FROM sqlite_master", function(err) {
        if (err) throw err;
        db.close(function(err) {
            assert.ok(err);
            error = true;
            assert.ok(err.message && err.message.indexOf("SQLITE_BUSY: unable to close due to") > -1);
            stmt.run();
        });
    });

    beforeExit(function() {
        assert.ok(error);
    });
};
