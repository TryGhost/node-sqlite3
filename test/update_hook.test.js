var sqlite3 = require('..');
var assert = require('assert');

describe('update_hook', function() {
    var db;

    beforeEach(function(done) {
        db = new sqlite3.Database(':memory:', function(err) {
            if (err) return done(err);

            db.run("CREATE TABLE update_hooks_test (id int PRIMARY KEY, value text)", done);
        });
    });

    it('emits insert event on inserting data to table', function(done) {
        db.addListener('change', function(eventType, database, table, rowId) {
            assert.equal(eventType, 'insert');
            assert.equal(database, 'main');
            assert.equal(table, 'update_hooks_test');
            assert.equal(rowId, 1);

            return done();
        });

        db.run("INSERT INTO update_hooks_test VALUES (1, 'value')", function(err) {
            if (err) return done(err);
        });
    });

    it('emits update event on row modification in table', function(done) {
        db.run("INSERT INTO update_hooks_test VALUES (2, 'value'), (3, 'value4')", function(err) {
            if (err) return done(err);

            db.addListener('change', function(eventType, database, table, rowId) {
                assert.equal(eventType, 'update');
                assert.equal(database, 'main');
                assert.equal(table, 'update_hooks_test');
                assert.equal(rowId, 1);

                db.all("SELECT * FROM update_hooks_test WHERE rowid = ?", rowId, function(err, rows) {
                    assert.deepEqual(rows, [{ id: 2, value: 'new_val' }]);

                    return done(err);
                });
            });

            db.run("UPDATE update_hooks_test SET value = 'new_val' WHERE id = 2", function(err) {
                if (err) return done(err);
            });
        });
    });

    it('emits delete event on row was deleted from table', function(done) {
        db.run("INSERT INTO update_hooks_test VALUES (2, 'value')", function(err) {
            if (err) return done(err);

            db.addListener('change', function(eventType, database, table, rowId) {
                assert.equal(eventType, 'delete');
                assert.equal(database, 'main');
                assert.equal(table, 'update_hooks_test');
                assert.equal(rowId, 1);

                return done();
            });

            db.run("DELETE FROM update_hooks_test WHERE id = 2", function(err) {
                if (err) return done(err);
            });
        });
    });

    afterEach(function(done) {
        db.close(done);
    });
});
