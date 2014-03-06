var sqlite3 = require('..');
var assert = require('assert');

describe('Should not crash', function() {

    it('unsafe access', function(done) {
        var sqlite3 = require('sqlite3');
        var db = new sqlite3.Database('foo.db')
        var o = { close: db.close };
        assert.throws(function () { o.close(); });
        done();
    });
});
