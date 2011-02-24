var sqlite3 = require('sqlite3');
var assert = require('assert');
var fs = require('fs');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test Database#exec'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');
    var finished = false;
    var sql = fs.readFileSync('test/support/script.sql', 'utf8');

    db.exec(sql, function(err) {
        if (err) throw err;

        db.all("SELECT type, name FROM sqlite_master ORDER BY type, name", function(err, rows) {
            if (err) throw err;

            assert.deepEqual(rows, [
                { type: 'index', name: 'grid_key_lookup' },
                { type: 'index', name: 'grid_utfgrid_lookup' },
                { type: 'index', name: 'images_id' },
                { type: 'index', name: 'keymap_lookup' },
                { type: 'index', name: 'map_index' },
                { type: 'index', name: 'name' },
                { type: 'table', name: 'grid_key' },
                { type: 'table', name: 'grid_utfgrid' },
                { type: 'table', name: 'images' },
                { type: 'table', name: 'keymap' },
                { type: 'table', name: 'map' },
                { type: 'table', name: 'metadata' },
                { type: 'view', name: 'grid_data' },
                { type: 'view', name: 'grids' },
                { type: 'view', name: 'tiles' }
            ]);

            finished = true;
        });
    });

    beforeExit(function() {
        assert.ok(finished);
    });
};
