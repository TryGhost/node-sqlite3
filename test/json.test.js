var sqlite3 = require('..');

describe('json', function() {
    var db;

    before(function(done) {
        db = new sqlite3.Database(':memory:', done);
    });

    it('should select JSON', function(done) {
        db.run('SELECT json(?)', JSON.stringify({ok:true}), done);
    });
});
