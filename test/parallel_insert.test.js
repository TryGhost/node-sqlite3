const sqlite3 = require('..');
const assert = require('assert');
const helper = require('./support/helper');

describe('parallel', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        helper.deleteFile('test/tmp/test_parallel_inserts.db');
        helper.ensureExists('test/tmp');
        db = await sqlite3.Database.create('test/tmp/test_parallel_inserts.db', );
    });

    const columns = [];
    for (let i = 0; i < 128; i++) {
        columns.push('id' + i);
    }
    
    it('should insert in parallel', async function() {
        await db.run("CREATE TABLE foo (" + columns + ")");
        for (let i = 0; i < 1000; i++) {
            const values = [];
            for (let j = 0; j < columns.length; j++) {
                values.push(i * j);
            }
            db.run("INSERT INTO foo VALUES (" + values + ")");
        }

        await db.wait();
        await db.close();
        assert.fileExists('test/tmp/test_parallel_inserts.db');
    });

    after(function() {
        helper.deleteFile('test/tmp/test_parallel_inserts.db');
    });
});
