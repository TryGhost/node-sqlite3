var sqlite3 = require('sqlite3');
var assert = require('assert');
var fs = require('fs');
var helper = require('./support/helper');

exports['open and close non-existent database'] = function(beforeExit) {
    var opened, closed;

    helper.deleteFile('test/tmp/test_create.db');
    var db = new sqlite3.Database('test/tmp/test_create.db', function(err) {
        if (err) throw err;
        assert.ok(!opened);
        assert.ok(!closed);
        opened = true;
    });

    db.close(function(err) {
        if (err) throw err;
        assert.ok(opened);
        assert.ok(!closed);
        closed = true;
    });

    beforeExit(function() {
        assert.ok(opened, 'Database not opened');
        assert.ok(closed, 'Database not closed');
        assert.fileExists('test/tmp/test_create.db');
        helper.deleteFile('test/tmp/test_create.db');
    });
};

exports['open inaccessible database'] = function(beforeExit) {
    var notOpened;

    var db = new sqlite3.Database('/usr/bin/test.db', function(err) {
        if (err && err.errno === sqlite3.CANTOPEN) {
            notOpened = true;
        }
        else if (err) throw err;
    });

    beforeExit(function() {
        assert.ok(notOpened, 'Database could be opened');
    });
};


exports['open non-existent database without create'] = function(beforeExit) {
    var notOpened;

    helper.deleteFile('tmp/test_readonly.db');
    var db = new sqlite3.Database('tmp/test_readonly.db', sqlite3.OPEN_READONLY,
        function(err) {
            if (err && err.errno === sqlite3.CANTOPEN) {
                notOpened = true;
            }
            else if (err) throw err;
        });

    beforeExit(function() {
        assert.ok(notOpened, 'Database could be opened');
        assert.fileDoesNotExist('tmp/test_readonly.db');
    });
};

exports['open and close memory database queuing'] = function(beforeExit) {
    var opened = 0, closed = 0, closeFailed = 0;

    var db = new sqlite3.Database(':memory:', function openedCallback(err) {
        if (err) throw err;
        opened++;
    });

    function closedCallback(err) {
        if (closed > 0) {
            assert.ok(err, 'No error object received on second close');
            assert.ok(err.errno === sqlite3.MISUSE);
            closeFailed++;
        }
        else if (err) throw err;
        else closed++;
    }

    db.close(closedCallback);
    db.close(closedCallback);

    beforeExit(function() {
        assert.equal(opened, 1, 'Database not opened');
        assert.equal(closed, 1, 'Database not closed');
        assert.equal(closeFailed, 1, 'Database could be closed again');
    });
};
