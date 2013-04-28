var sqlite3 = require('..');
var assert = require('assert');

describe('prepare', function() {
    describe('invalid SQL', function() {
        var db;
        before(function(done) { db = new sqlite3.Database(':memory:', done); });

        var stmt;
        it('should fail preparing a statement with invalid SQL', function(done) {
            stmt = db.prepare('CRATE TALE foo text bar)', function(err, statement) {
                if (err && err.errno == sqlite3.ERROR &&
                    err.message === 'SQLITE_ERROR: near "CRATE": syntax error') {
                    done();
                }
                else throw err;
            });
        });

        after(function(done) { db.close(done); });
    });

    describe('simple prepared statement', function() {
        var db;
        before(function(done) { db = new sqlite3.Database(':memory:', done); });

        it('should prepare, run and finalize the statement', function(done) {
            db.prepare("CREATE TABLE foo (text bar)")
                .run()
                .finalize(done);
        });

        after(function(done) { db.close(done); });
    });

    describe('inserting and retrieving rows', function() {
        var db;
        before(function(done) { db = new sqlite3.Database(':memory:', done); });

        var inserted = 0;
        var retrieved = 0;

        // We insert and retrieve that many rows.
        var count = 1000;

        it('should create the table', function(done) {
            db.prepare("CREATE TABLE foo (txt text, num int, flt float, blb blob)").run().finalize(done);
        });

        it('should insert ' + count + ' rows', function(done) {
            for (var i = 0; i < count; i++) {
                db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)").run(
                    'String ' + i,
                    i,
                    i * Math.PI,
                    // null (SQLite sets this implicitly)
                    function(err) {
                        if (err) throw err;
                        inserted++;
                    }
                ).finalize(function(err) {
                    if (err) throw err;
                    if (inserted == count) done();
                });
            }
        });

        it('should prepare a statement and run it ' + (count + 5) + ' times', function(done) {
            var stmt = db.prepare("SELECT txt, num, flt, blb FROM foo ORDER BY num", function(err) {
                if (err) throw err;
                assert.equal(stmt.sql, 'SELECT txt, num, flt, blb FROM foo ORDER BY num');
            });

            for (var i = 0; i < count + 5; i++) (function(i) {
                stmt.get(function(err, row) {
                    if (err) throw err;

                    if (retrieved >= 1000) {
                        assert.equal(row, undefined);
                    } else {
                        assert.equal(row.txt, 'String ' + i);
                        assert.equal(row.num, i);
                        assert.equal(row.flt, i * Math.PI);
                        assert.equal(row.blb, null);
                    }

                    retrieved++;
                });
            })(i);

            stmt.finalize(done);
        });

        it('should have retrieved ' + (count + 5) + ' rows', function() {
            assert.equal(count + 5, retrieved, "Didn't retrieve all rows");
        });


        after(function(done) { db.close(done); });
    });

    describe('retrieving reset() function', function() {
        var db;
        before(function(done) { db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY, done); });

        var retrieved = 0;

        it('should retrieve the same row over and over again', function(done) {
            var stmt = db.prepare("SELECT txt, num, flt, blb FROM foo ORDER BY num");
            for (var i = 0; i < 10; i++) {
                stmt.reset();
                stmt.get(function(err, row) {
                    if (err) throw err;
                    assert.equal(row.txt, 'String 0');
                    assert.equal(row.num, 0);
                    assert.equal(row.flt, 0.0);
                    assert.equal(row.blb, null);
                    retrieved++;
                });
            }
            stmt.finalize(done);
        });

        it('should have retrieved 10 rows', function() {
            assert.equal(10, retrieved, "Didn't retrieve all rows");
        });

        after(function(done) { db.close(done); });
    });

    describe('multiple get() parameter binding', function() {
        var db;
        before(function(done) { db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY, done); });

        var retrieved = 0;

        it('should retrieve particular rows', function(done) {
            var stmt = db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num = ?");

            for (var i = 0; i < 10; i++) (function(i) {
                stmt.get(i * 10 + 1, function(err, row) {
                    if (err) throw err;
                    var val = i * 10 + 1;
                    assert.equal(row.txt, 'String ' + val);
                    assert.equal(row.num, val);
                    assert.equal(row.flt, val * Math.PI);
                    assert.equal(row.blb, null);
                    retrieved++;
                });
            })(i);

            stmt.finalize(done);
        });

        it('should have retrieved 10 rows', function() {
            assert.equal(10, retrieved, "Didn't retrieve all rows");
        });

        after(function(done) { db.close(done); });
    });

    describe('prepare() parameter binding', function() {
        var db;
        before(function(done) { db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY, done); });

        var retrieved = 0;

        it('should retrieve particular rows', function(done) {
            db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num = ? AND txt = ?", 10, 'String 10')
                .get(function(err, row) {
                    if (err) throw err;
                    assert.equal(row.txt, 'String 10');
                    assert.equal(row.num, 10);
                    assert.equal(row.flt, 10 * Math.PI);
                    assert.equal(row.blb, null);
                    retrieved++;
                })
                .finalize(done);
        });

        it('should have retrieved 1 row', function() {
            assert.equal(1, retrieved, "Didn't retrieve all rows");
        });

        after(function(done) { db.close(done); });
    });

    describe('all()', function() {
        var db;
        before(function(done) { db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY, done); });

        var retrieved = 0;
        var count = 1000;

        it('should retrieve particular rows', function(done) {
            db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num < ? ORDER BY num", count)
                .all(function(err, rows) {
                    if (err) throw err;
                    for (var i = 0; i < rows.length; i++) {
                        assert.equal(rows[i].txt, 'String ' + i);
                        assert.equal(rows[i].num, i);
                        assert.equal(rows[i].flt, i * Math.PI);
                        assert.equal(rows[i].blb, null);
                        retrieved++;
                    }
                })
                .finalize(done);
        });

        it('should have retrieved all rows', function() {
            assert.equal(count, retrieved, "Didn't retrieve all rows");
        });

        after(function(done) { db.close(done); });
    });

    describe('all()', function() {
        var db;
        before(function(done) { db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY, done); });

        it('should retrieve particular rows', function(done) {
           db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num > 5000")
                .all(function(err, rows) {
                    if (err) throw err;
                    assert.ok(rows.length === 0);
                })
                .finalize(done);
        });

        after(function(done) { db.close(done); });
    });

    describe('high concurrency', function() {
        var db;
        before(function(done) { db = new sqlite3.Database(':memory:', done); });

        function randomString() {
            var str = '';
            for (var i = Math.random() * 300; i > 0; i--) {
                str += String.fromCharCode(Math.floor(Math.random() * 256));
            }
            return str;
        }

        // Generate random data.
        var data = [];
        var length = Math.floor(Math.random() * 1000) + 200;
        for (var i = 0; i < length; i++) {
            data.push([ randomString(), i, i * Math.random(), null ]);
        }

        var inserted = 0;
        var retrieved = 0;

        it('should create the table', function(done) {
            db.prepare("CREATE TABLE foo (txt text, num int, flt float, blb blob)").run().finalize(done);
        });

        it('should insert all values', function(done) {
            for (var i = 0; i < data.length; i++) {
                var stmt = db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
                stmt.run(data[i][0], data[i][1], data[i][2], data[i][3], function(err) {
                    if (err) throw err;
                    inserted++;
                }).finalize(function(err) {
                    if (err) throw err;
                    if (inserted == data.length) done();
                });
            }
        });

        it('should retrieve all values', function(done) {
            db.prepare("SELECT txt, num, flt, blb FROM foo")
                .all(function(err, rows) {
                    if (err) throw err;

                    for (var i = 0; i < rows.length; i++) {
                        assert.ok(data[rows[i].num] !== true);

                        assert.equal(rows[i].txt, data[rows[i].num][0]);
                        assert.equal(rows[i].num, data[rows[i].num][1]);
                        assert.equal(rows[i].flt, data[rows[i].num][2]);
                        assert.equal(rows[i].blb, data[rows[i].num][3]);

                        // Mark the data row as already retrieved.
                        data[rows[i].num] = true;
                        retrieved++;

                    }

                    assert.equal(retrieved, data.length);
                    assert.equal(retrieved, inserted);
                })
                .finalize(done);
        });

        after(function(done) { db.close(done); });
    });


    describe('test Database#get()', function() {
        var db;
        before(function(done) { db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY, done); });

        var retrieved = 0;

        it('should get a row', function(done) {
            db.get("SELECT txt, num, flt, blb FROM foo WHERE num = ? AND txt = ?", 10, 'String 10', function(err, row) {
                if (err) throw err;
                assert.equal(row.txt, 'String 10');
                assert.equal(row.num, 10);
                assert.equal(row.flt, 10 * Math.PI);
                assert.equal(row.blb, null);
                retrieved++;
                done();
            });
        });

        it('should have retrieved all rows', function() {
            assert.equal(1, retrieved, "Didn't retrieve all rows");
        });

        after(function(done) { db.close(done); });
    });

    describe('Database#run() and Database#all()', function() {
        var db;
        before(function(done) { db = new sqlite3.Database(':memory:', done); });

        var inserted = 0;
        var retrieved = 0;

        // We insert and retrieve that many rows.
        var count = 1000;

        it('should create the table', function(done) {
            db.run("CREATE TABLE foo (txt text, num int, flt float, blb blob)", done);
        });

        it('should insert ' + count + ' rows', function(done) {
            for (var i = 0; i < count; i++) {
                db.run("INSERT INTO foo VALUES(?, ?, ?, ?)",
                    'String ' + i,
                    i,
                    i * Math.PI,
                    // null (SQLite sets this implicitly)
                    function(err) {
                        if (err) throw err;
                        inserted++;
                        if (inserted == count) done();
                    }
                );
            }
        });

        it('should retrieve all rows', function(done) {
            db.all("SELECT txt, num, flt, blb FROM foo ORDER BY num", function(err, rows) {
                if (err) throw err;
                for (var i = 0; i < rows.length; i++) {
                    assert.equal(rows[i].txt, 'String ' + i);
                    assert.equal(rows[i].num, i);
                    assert.equal(rows[i].flt, i * Math.PI);
                    assert.equal(rows[i].blb, null);
                    retrieved++;
                }

                assert.equal(retrieved, count);
                assert.equal(retrieved, inserted);

                done();
            });
        });

        after(function(done) { db.close(done); });
    });
});
