const sqlite3 = require('..');
const assert = require('assert');

describe('tracing', function() {
    it('Database tracing', async function() {
        const db = await sqlite3.Database.create(':memory:');
        let create = false;
        let select = false;

        db.on('trace', function(sql) {
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

        await db.serialize(async function() {
            await db.run("CREATE TABLE foo (id int)");
            await db.run("SELECT * FROM foo");
        });

        await db.close();
        assert.ok(create);
        assert.ok(select);
    });


    it('test disabling tracing #1', async function() {
        const db = await sqlite3.Database.create(':memory:');

        db.on('trace', function() {});
        db.removeAllListeners('trace');
        db._events['trace'] = function() {
            assert.ok(false);
        };

        await db.run("CREATE TABLE foo (id int)");
        await db.close();
    });


    it('test disabling tracing #2', async function() {
        const db = await sqlite3.Database.create(':memory:');

        const trace = function() {};
        db.on('trace', trace);
        db.removeListener('trace', trace);
        db._events['trace'] = function() {
            assert.ok(false);
        };

        await db.run("CREATE TABLE foo (id int)");
        await db.close();
    });
});
