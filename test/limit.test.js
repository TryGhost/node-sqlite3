var sqlite3 = require('..');

describe('limit', function() {
    var db;

    before(function(done) {
        db = new sqlite3.Database(':memory:', done);
    });

    it('should support applying limits via configure', function(done) {
        db.configure('limit', sqlite3.LIMIT_ATTACHED, 0);
        db.exec("ATTACH 'test/support/prepare.db' AS zing", function(err) {
            if (!err) {
                throw new Error('ATTACH should not succeed');
            }
            if (err.errno === sqlite3.ERROR &&
                err.message === 'SQLITE_ERROR: too many attached databases - max 0') {
                db.configure('limit', sqlite3.LIMIT_ATTACHED, 1);
                db.exec("ATTACH 'test/support/prepare.db' AS zing", function(err) {
                    if (err) throw err;
                    db.close(done);
                });
            } else {
                throw err;
            }
        });
    });
});
