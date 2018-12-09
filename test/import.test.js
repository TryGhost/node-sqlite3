var sqlite3 = require('..');
var assert = require('assert');

describe('import', function() {
    var db;
    before(function(done) {
        db = new sqlite3.Database(':memory:', sqlite3.OPEN_READWRITE, done);
    });

    it('basic import', function(done) {
        db.import('test/support/import/sample.csv', 'sample', {}, function (err, res) {
            if (err) throw err;
            assert.deepEqual(res, {
                tableName: 'sample',
                columnIds: ['firstName', 'lastName', 'email', 'phoneNumber'],
                columnTypes: ['text', 'text', 'text', 'integer'],
                rowCount: 3
            });
            done();
        })
    });

    it('should error when import invalid filename', function(done) {
        db.import('/an/invalid/path', 'sample', {}, function (err, res) {
            if (err) {
                assert.equal(err.message, 'SQLITE_ERROR: cannot open file "/an/invalid/path"');
                assert.equal(err.errno, sqlite3.ERROR);
                assert.equal(err.code, 'SQLITE_ERROR');
                done();
            } else {
                done(new Error('Completed import without error, but expected error'));
            }
        });
    });
});
