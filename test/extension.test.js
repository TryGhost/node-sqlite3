var sqlite3 = require('sqlite3');
var assert = require('assert');
var path = require('path');

if (process.setMaxListeners) process.setMaxListeners(0);

var spatialite_ext = '/usr/local/lib/libspatialite.dylib';

exports['test loadExtension'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');
    var completed = false;

    if (path.existsSync(spatialite_ext)) {
        db.loadExtension(spatialite_ext, function(err) {
            if (err) throw err;
            completed = true;
        });
    } else {
        completed = true;
    }

    beforeExit(function() {
        assert.ok(completed);
    });
}
