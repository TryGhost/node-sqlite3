var sqlite3 = require('..');
var assert = require('assert');
var fs = require('fs');
var helper = require('./support/helper');


describe('configure/chunkSize', function() {
    var DBNAME = 'test/tmp/chunk_size.db';

    var db;

    var populateWithData = function(db, count) {
        var randomString = function() {
            var str = '';
            var chars = 'abcdefghijklmnopqrstuvwxzyABCDEFGHIJKLMNOPQRSTUVWXZY0123456789  ';
            for (var i = 93; i > 0; i--) {
                str += chars[Math.floor(Math.random() * chars.length)];
            }
            return str;
        };
        db.serialize(function() {
            db.run("CREATE TABLE foo (id INT, txt TEXT)");
            db.run("BEGIN TRANSACTION");
            var stmt = db.prepare("INSERT INTO foo VALUES(?, ?)");
            for (var i = 0; i < count; i++) {
                stmt.run(i, randomString());
            }
            stmt.finalize();
            db.run("COMMIT TRANSACTION");
        });
    };

    beforeEach(function(done) {
        helper.ensureExists('test/tmp');
        helper.deleteFile(DBNAME);
        assert.fileDoesNotExist(DBNAME);
        db = new sqlite3.Database(DBNAME, done);
    });

    afterEach(function(done) {
        var deleteTheFile = function() {
            helper.deleteFile(DBNAME);
            done();
        };
        db.close(deleteTheFile);
    });

    it('should throw an error if chunkSize < 1', function() {
        var fn = function() {
            db.configure('chunkSize', -1);
        };
        assert.throws(fn, /Value must be an integer > 0 and must be a power of 2/);
    });

    it('should throw an error if chunkSize is not an int', function() {
        var fn = function() {
            db.configure('chunkSize', 'what are you doing?');
        };
        assert.throws(fn, /Value must be an integer > 0 and must be a power of 2/);
    });

    it('should throw an error if chunkSize is not a multiple of 2', function() {
        var fn = function() {
            db.configure('chunkSize', 3);
        };
        assert.throws(fn, /Value must be an integer > 0 and must be a power of 2/);
    });

    it('should create a db file which is a multiple of chunkSize', function(done) {
        var chunkSize = Math.pow(2, 16);

        var verifyAssertions = function() {
            var stats = fs.statSync(DBNAME);
            assert.ok(stats.size > 0);
            assert.equal(stats.size % chunkSize, 0);
            assert.equal(stats.size, chunkSize);
            done();
        };

        var populateAndClose = function() {
            // with only 100 rows of data the size of the data
            // is pretty small but the chunksize is large so
            // the .db file is the chunk size
            populateWithData(db, 100);
            db.close(verifyAssertions);
        };

        db.configure('chunkSize', chunkSize, populateAndClose);
    });
});
