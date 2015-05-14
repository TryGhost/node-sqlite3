var sqlite3 = require('..');
var assert = require('assert');

describe('declared types', function() {
    var db;
    before(function(done) {
        db = new sqlite3.Database(':memory:', done);
    });

    it('should create the table', function(done) {
        db.run('CREATE TABLE foo (txt TEXT, num INT, date DATETIME)', done);
    });

    it('should retrieve declared types in all', function(done) {
        db.all('SELECT *, 1 as untyped FROM foo', function(err, rows, decltypes) {
            if (err) throw err;
            assert.deepEqual(decltypes, {
                num: 'INT',
                txt: 'TEXT',
                date: 'DATETIME'
            });
            done();
        });
    });

    it('should retrieve declared types in get', function(done) {
        db.get('SELECT *, 1 as untyped FROM foo', function(err, row, decltypes) {
            if (err) throw err;
            assert.deepEqual(decltypes, {
                num: 'INT',
                txt: 'TEXT',
                date: 'DATETIME'
            });
            done();
        });
    });

    it('should insert a value', function(done) {
        db.run('INSERT INTO foo VALUES("hello world", 1, "2015-01-01")', done);
    });

    it('should retrieve declared types in each', function(done) {
        db.each('SELECT *, 1 as untyped FROM foo', function(err, rows, decltypes) {
            if (err) throw err;
            assert.deepEqual(decltypes, {
                num: 'INT',
                txt: 'TEXT',
                date: 'DATETIME'
            });
            done();
        });
    });
});
