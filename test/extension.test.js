var sqlite3 = require('sqlite3');
var assert = require('assert');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test loadExtension'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');
    var completed = false;

    db.loadExtension('/usr/local/lib/libspatialite.dylib', function(err) {
        if (err) throw err;
        completed = true;
    });

    beforeExit(function() {
        assert.ok(completed);
    });
}
