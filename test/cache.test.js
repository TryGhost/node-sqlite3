const sqlite3 = require('..');
const assert = require('assert');
const helper = require('./support/helper');

describe('cache', function() {
    before(function() {
        helper.ensureExists('test/tmp');
    });

    it('should cache Database objects while opening', async function() {
        const filename = 'test/tmp/test_cache.db';
        helper.deleteFile(filename);
        const db1 = await sqlite3.cached.Database.create(filename);
        const db2 = await sqlite3.cached.Database.create(filename);
        assert.equal(db1, db2);
    });

    // TODO: Does this still make sense? Is this correctly implemented now?
    it('should cache Database objects after they are open', function(done) {
        const filename = 'test/tmp/test_cache2.db';
        helper.deleteFile(filename);
        sqlite3.cached.Database.create(filename).then((db1) => {
            process.nextTick(function() {
                sqlite3.cached.Database.create(filename).then((db2)  =>{
                    assert.equal(db1, db2);
                    done();
                });
            });
        });
    });
});
