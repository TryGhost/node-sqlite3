var sqlite3 = require('sqlite3');
var assert = require('assert');
var exists = require('fs').existsSync;


// disabled because this is not a generically safe test to run on all systems

var spatialite_ext = '/usr/local/lib/libspatialite.dylib';

describe('loadExtension', function() {
    var db;
    beforeAll(function(done) {
        db = new sqlite3.Database(':memory:', done);
    });

    if (exists(spatialite_ext)) {
        it.skip('libspatialite', function(done) {
            db.loadExtension(spatialite_ext, done);
        });
    } else {
        it('libspatialite', function() {
        });
    }
});
