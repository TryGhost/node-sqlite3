var sqlite3 = require('..');
var assert = require('assert');

describe('backup', function() {
    var db;
    before(function(done) {
        db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY, done);
    });

    it('backup database', function(done) {
        db.backup('test/support/backup.db', done);
    });
});

