var sqlite3 = require('sqlite3');
var assert = require('assert');
var util = require('util');
var helper = require('./support/helper');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test parallel inserts'] = function(beforeExit) {
    var completed = false;
    helper.deleteFile('test/tmp/test_parallel_inserts.db');
    var db = new sqlite3.Database('test/tmp/test_parallel_inserts.db');

    var columns = [];
    for (var i = 0; i < 128; i++) {
        columns.push('id' + i);
    }

    db.serialize(function() {
        db.run("CREATE TABLE foo (" + columns + ")");
    });

    for (var i = 0; i < 10000; i++) {
        for (var values = [], j = 0; j < columns.length; j++) {
            values.push(i * j);
        }
        db.run("INSERT INTO foo VALUES (" + values + ")");
    }

    db.close(function() {
        completed = true;
    });

    beforeExit(function() {
        assert.ok(completed);
        assert.fileExists('test/tmp/test_parallel_inserts.db');
        helper.deleteFile('test/tmp/test_parallel_inserts.db');
    });
};
