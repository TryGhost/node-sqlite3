const sqlite3 = require('..');
const assert = require('assert');
const fs = require('fs');

describe('exec', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
    });

    it('Database#exec', async function() {
        const sql = fs.readFileSync('test/support/script.sql', 'utf8');
        await db.exec(sql);
    });

    it('retrieve database structure', async function() {
        const rows = await db.all("SELECT type, name FROM sqlite_master ORDER BY type, name");
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
    });
});
