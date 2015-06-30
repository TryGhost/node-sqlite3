var sqlite3 = require('..');
var assert = require('assert');

describe('user functions', function() {
    var db;
    before(function(done) { db = new sqlite3.Database(':memory:', done); });

    it('should allow registration of user functions', function() {
        db.registerFunction('MY_UPPERCASE', function(value) {
            return value.toUpperCase();
        });
        db.registerFunction('MY_STRING_JOIN', function(value1, value2) {
            return [value1, value2].join(' ');
        });
        db.registerFunction('MY_Add', function(value1, value2) {
            return value1 + value2;
        });
        db.registerFunction('MY_REGEX', function(regex, value) {
            return !!value.match(new RegExp(regex));
        });
        db.registerFunction('MY_REGEX_VALUE', function(regex, value) {
            return /match things/i;
        });
        db.registerFunction('MY_ERROR', function(value) {
            throw new Error('This function always throws');
        });
        db.registerFunction('MY_UNHANDLED_TYPE', function(value) {
            return {};
        });
        db.registerFunction('MY_NOTHING', function(value) {

        });
    });

    it('should process user functions with one arg', function(done) {
        db.all('SELECT MY_UPPERCASE("hello") AS txt', function(err, rows) {
            if (err) throw err;
            assert.equal(rows.length, 1);
            assert.equal(rows[0].txt, 'HELLO')
            done();
        });
    });

    it('should process user functions with two args', function(done) {
        db.all('SELECT MY_STRING_JOIN("hello", "world") AS val', function(err, rows) {
            if (err) throw err;
            assert.equal(rows.length, 1);
            assert.equal(rows[0].val, 'hello world');
            done();
        });
    });

    it('should process user functions with number args', function(done) {
        db.all('SELECT MY_ADD(1, 2) AS val', function(err, rows) {
            if (err) throw err;
            assert.equal(rows.length, 1);
            assert.equal(rows[0].val, 3);
            done();
        });
    });

    it('allows writing of a regex function', function(done) {
        db.all('SELECT MY_REGEX("colou?r", "color") AS val', function(err, rows) {
            if (err) throw err;
            assert.equal(rows.length, 1);
            assert.equal(Boolean(rows[0].val), true);
            done();
        });
    });

    it('converts returned regex instances to strings', function(done) {
        db.all('SELECT MY_REGEX_VALUE() AS val', function(err, rows) {
            if (err) throw err;
            assert.equal(rows.length, 1);
            assert.equal(rows[0].val, '/match things/i');
            done();
        });
    });

    it('reports errors thrown in functions', function(done) {
        db.all('SELECT MY_ERROR() AS val', function(err, rows) {
            assert.equal(err.message, 'SQLITE_ERROR: Uncaught Error: This function always throws');
            assert.equal(rows, undefined);
            done();
        });
    });

    it('reports errors for unhandled types', function(done) {
        db.all('SELECT MY_UNHANDLED_TYPE() AS val', function(err, rows) {
            assert.equal(err.message, 'SQLITE_ERROR: invalid return type in ' +
                'user function MY_UNHANDLED_TYPE');
            assert.equal(rows, undefined);
            done();
        });
    });

    it('allows no return value from functions', function(done) {
        db.all('SELECT MY_NOTHING() AS val', function(err, rows) {
            if (err) throw err;
            assert.equal(rows.length, 1);
            assert.equal(rows[0].val, undefined);
            done();
        });
    });

    after(function(done) { db.close(done); });
});
