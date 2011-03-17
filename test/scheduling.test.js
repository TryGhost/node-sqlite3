var sqlite3 = require('sqlite3');
var assert = require('assert');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test scheduling a query after the database was closed'] = function(beforeExit) {
    var error = false;
    var db = new sqlite3.Database(':memory:');
    db.on('error', function(err) {
        error = true;
        assert.equal(err.message, "SQLITE_MISUSE: Database handle is closed");
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
        assert.ok(err.message, "SQLITE_MISUSE: Database handle is closed");
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
    db.on('error', function(err) {
        error = true;
        assert.equal(err.message, "SQLITE_BUSY: unable to close due to unfinalised statements");
    });

    var stmt = db.prepare("CREATE TABLE foo (id int)");
    db.close();
    stmt.run();

    beforeExit(function() {
        assert.ok(error);
    });
};
