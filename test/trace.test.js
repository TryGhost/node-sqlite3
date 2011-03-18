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

// TODO: test turning tracing off again
