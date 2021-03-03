var sqlite3 = require('..');
var assert = require('assert');

if( process.env.NODE_SQLITE3_JSON1 === 'no' ){
    describe('extension_functions', function() {
        it(
            'skips extension_functions tests when --sqlite=/usr (or similar) is tested',
            function(){}
        );
    });
} else {
    describe('extension_functions', function() {
        var db;

        before(function(done) {
            db = new sqlite3.Database(':memory:', done);
        });

        it('should select sqrt(9) as 3', function(done) {
            db.all('SELECT sqrt(9) AS val', function(err, rows) {
                if (err) {
                    throw err;
                }
                assert.equal(rows[0].val, 3);
                done();
            });
        });
    });
}
