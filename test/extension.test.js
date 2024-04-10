// let sqlite3 = require('..');
// let assert = require('assert');
// let exists = require('fs').existsSync || require('path').existsSync;

/*

// disabled because this is not a generically safe test to run on all systems

var spatialite_ext = '/usr/local/lib/libspatialite.dylib';

describe('loadExtension', function(done) {
    var db;
    before(function(done) {
        db = new sqlite3.Database(':memory:', done);
    });

    if (exists(spatialite_ext)) {
        it('libspatialite', function(done) {
            db.loadExtension(spatialite_ext, done);
        });
    } else {
        it('libspatialite');
    }
});

*/
