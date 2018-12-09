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
        });
    });

    it('import TSV using delimiter option', function(done) {
        db.import('test/support/import/data.tsv', 'data', { delimiter: '\t' }, function (err, res) {
            if (err) throw err;
            assert.deepEqual(res, {
                tableName: 'data',
                columnIds: ['x', 'y'],
                columnTypes: ['integer', 'integer'],
                rowCount: 5
            });
            done();
        });
    });    

    it('import with noHeaderRow and columnIds options', function (done) {
        const columnIds = ['col1', 'col2', 'col3', 'col4']
        db.import('test/support/import/sample-no-header.csv', 'sampleNoHeader',
            { columnIds, noHeaderRow: true }, function (err, res) {
                if (err) throw err;
                assert.deepEqual(res, {
                    tableName: 'sampleNoHeader',
                    columnIds,
                    columnTypes: ['text', 'text', 'text', 'integer'],
                    rowCount: 3
                });
                done();
            });
    });

    it('should error on import with invalid filename', function(done) {
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
