const sqlite3 = require('..');
const assert = require('assert');

describe('profiling', function() {
    let create = false;
    let select = false;

    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');

        db.on('profile', function(sql, nsecs) {
            assert.ok(typeof nsecs === "number");
            if (sql.match(/^SELECT/)) {
                assert.ok(!select);
                assert.equal(sql, "SELECT * FROM foo");
                select = true;
            }
            else if (sql.match(/^CREATE/)) {
                assert.ok(!create);
                assert.equal(sql, "CREATE TABLE foo (id int)");
                create = true;
            }
            else {
                assert.ok(false);
            }
        });
    });

    it('should profile a create table', function(done) {
        assert.ok(!create);
        db.run("CREATE TABLE foo (id int)").then(() => {
            setImmediate(() => {
                assert.ok(create);
                done();
            });
        });
    });


    it('should profile a select', function(done) {
        assert.ok(!select);
        db.run("SELECT * FROM foo").then(() => {
            setImmediate(function() {
                assert.ok(select);
                done();
            });
        });
    });

    after(async function() {
        await db.close();
    });
});
