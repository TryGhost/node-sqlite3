var sqlite3 = require('sqlite3');
var assert = require('assert');
var util = require('util');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test Database tracing'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');
    var create = false;
    var select = false;

    db.on('trace', function(sql) {
        if (sql.match(/^SELECT/)) {
            assert.ok(!select);
            assert.equal(sql, "SELECT * FROM foo");
            select = true;
        }
        else if (sql.match(/^CREATE/)) {
            assert.ok(!create);
            assert.equal(sql, "CREATE TABLE foo (id int)");
            create = true;
        }
        else {
            assert.ok(false);
        }
    });

    db.serialize(function() {
        db.run("CREATE TABLE foo (id int)");
        db.run("SELECT * FROM foo");
    });

    db.close();

    beforeExit(function() {
        assert.ok(create);
        assert.ok(select);
    });
};

exports['test disabling tracing #1'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    db.on('trace', function(sql) {});
    db.removeAllListeners('trace');
    db._events['trace'] = function(sql) {
        assert.ok(false);
    };

    db.run("CREATE TABLE foo (id int)");
    db.close();
};

exports['test disabling tracing #2'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var trace = function(sql) {};
    db.on('trace', trace);
    db.removeListener('trace', trace);
    db._events['trace'] = function(sql) {
        assert.ok(false);
    };

    db.run("CREATE TABLE foo (id int)");
    db.close();
};
