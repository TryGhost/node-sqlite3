var sqlite3 = require('sqlite3');
var assert = require('assert');
var helper = require('./support/helper');

describe('parallel', function() {
    var db;
    beforeAll(function(done) {
        helper.deleteFile('test/tmp/test_parallel_inserts.db');
        helper.ensureExists('test/tmp');
        db = new sqlite3.Database('test/tmp/test_parallel_inserts.db', done);
    });

    var columns = [];
    for (var i = 0; i < 128; i++) {
        columns.push('id' + i);
    }

    it('should create the table', function(done) {
        db.run("CREATE TABLE foo (" + columns + ")", done);
    });

    it('should insert in parallel', function(done) {
        for (var i = 0; i < 1000; i++) {
            for (var values = [], j = 0; j < columns.length; j++) {
                values.push(i * j);
            }
            db.run("INSERT INTO foo VALUES (" + values + ")");
        }

        db.wait(done);
    }, 480000);

    it('should close the database', function(done) {
        db.close(done);
    });

    it('should verify that the database exists', function() {
        helper.fileExists('test/tmp/test_parallel_inserts.db');
    });

    afterAll(function() {
        helper.deleteFile('test/tmp/test_parallel_inserts.db');
    });
});
