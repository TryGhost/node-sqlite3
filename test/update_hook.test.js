const sqlite3 = require('..');
const assert = require('assert');

describe('update_hook', function() {
    /** @type {sqlite3.Database} */
    let db;

    beforeEach(async function() {
        db = await sqlite3.Database.create(':memory:');
        await db.run("CREATE TABLE update_hooks_test (id int PRIMARY KEY, value text)");
    });

    it('emits insert event on inserting data to table', function(done) {
        db.addListener('change', function(eventType, database, table, rowId) {
            assert.equal(eventType, 'insert');
            assert.equal(database, 'main');
            assert.equal(table, 'update_hooks_test');
            assert.equal(rowId, 1);

            return done();
        });

        db.run("INSERT INTO update_hooks_test VALUES (1, 'value')")
            .catch((err) => done(err));
    });

    it('emits update event on row modification in table', function(done) {
        db.run("INSERT INTO update_hooks_test VALUES (2, 'value'), (3, 'value4')")
            .catch((err) => done(err))
            .then(() => {
                db.addListener('change', function(eventType, database, table, rowId) {
                    assert.equal(eventType, 'update');
                    assert.equal(database, 'main');
                    assert.equal(table, 'update_hooks_test');
                    assert.equal(rowId, 1);

                    db.all("SELECT * FROM update_hooks_test WHERE rowid = ?", rowId)
                        .catch((err) => done(err))
                        .then((rows) => {
                            assert.deepEqual(rows, [{ id: 2, value: 'new_val' }]);

                            return done();
                        });
                });

                db.run("UPDATE update_hooks_test SET value = 'new_val' WHERE id = 2")
                    .catch((err) => done(err));
            });
    });

    it('emits delete event on row was deleted from table', function(done) {
        db.run("INSERT INTO update_hooks_test VALUES (2, 'value')")
            .catch((err) => done(err))
            .then(() => {
                db.addListener('change', function(eventType, database, table, rowId) {
                    assert.equal(eventType, 'delete');
                    assert.equal(database, 'main');
                    assert.equal(table, 'update_hooks_test');
                    assert.equal(rowId, 1);
                });

                db.run("DELETE FROM update_hooks_test WHERE id = 2")
                    .catch((err) => done(err)).then(() => done());
            });
    });

    afterEach(async function() {
        await db.close();
    });
});
