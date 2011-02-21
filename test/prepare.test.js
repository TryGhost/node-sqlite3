var sqlite3 = require('sqlite3');
var assert = require('assert');
var Step = require('step');

exports['test simple prepared statement with invalid SQL'] = function(beforeExit) {
    var error = false;
    var db = new sqlite3.Database(':memory:');
    db.prepare('CRATE TALE foo text bar)', function(err, statement) {
        if (err && err.errno == sqlite3.ERROR &&
            err.message === 'SQLITE_ERROR: near "CRATE": syntax error') {
            error = true;
        }
        else throw err;
    }).finalize();
    db.close();

    beforeExit(function() {
        assert.ok(error, 'No error thrown for invalid SQL');
    });
};

exports['test simple prepared statement'] = function(beforeExit) {
    var run = false;
    var db = new sqlite3.Database(':memory:');
    db.prepare("CREATE TABLE foo (text bar)")
        .run(function(err) {
            if (err) throw err;
            run = true;
        })
        .finalize()
    .close();

    beforeExit(function() {
        assert.ok(run, 'Database query did not run');
    });
};

exports['test inserting and retrieving rows'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var inserted = 0;
    var retrieved = 0;

    // We insert and retrieve that many rows.
    var count = 1000;

    Step(
        function() {
            db.prepare("CREATE TABLE foo (txt text, num int, flt float, blb blob)").run(this);
        },
        function(err) {
            if (err) throw err;
            var group = this.group();
            for (var i = 0; i < count; i++) {
                db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)")
                  .run(
                      'String ' + i,
                      i,
                      i * Math.PI,
                      // null (SQLite sets this implicitly)
                      group()
                  );
            }
        },
        function(err, rows) {
            if (err) throw err;
            inserted += rows.length;
            var stmt = db.prepare("SELECT txt, num, flt, blb FROM foo ORDER BY num");

            var group = this.group();
            for (var i = 0; i < count + 5; i++) {
                stmt.get(group());
            }
        },
        function(err, rows) {
            if (err) throw err;
            assert.equal(count + 5, rows.length, "Didn't run all queries");

            for (var i = 0; i < count; i++) {
                assert.equal(rows[i][0], 'String ' + i);
                assert.equal(rows[i][1], i);
                assert.equal(rows[i][2], i * Math.PI);
                assert.equal(rows[i][3], null);
                retrieved++;
            }

            for (var i = rows; i < count + 5; i++) {
                assert.ok(!rows[i]);
            }
        }
    );

    beforeExit(function() {
        assert.equal(count, inserted, "Didn't insert all rows");
        assert.equal(count, retrieved, "Didn't retrieve all rows");
    });
};

exports['test retrieving reset() function'] = function(beforeExit) {
    var db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY);

    var retrieved = 0;

    Step(
        function() {
            var stmt = db.prepare("SELECT txt, num, flt, blb FROM foo ORDER BY num");

            var group = this.group();
            for (var i = 0; i < 10; i++) {
                stmt.reset();
                stmt.get(group());
            }
        },
        function(err, rows) {
            if (err) throw err;
            for (var i = 0; i < rows.length; i++) {
                assert.equal(rows[i][0], 'String 0');
                assert.equal(rows[i][1], 0);
                assert.equal(rows[i][2], 0.0);
                assert.equal(rows[i][3], null);
                retrieved++;
            }
        }
    );

    beforeExit(function() {
        assert.equal(10, retrieved, "Didn't retrieve all rows");
    });
};

exports['test multiple get() parameter binding'] = function(beforeExit) {
    var db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY);

    var retrieved = 0;

    Step(
        function() {
            var stmt = db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num = ?");

            var group = this.group();
            for (var i = 0; i < 10; i++) {
                stmt.get(i * 10 + 1, group());
            }
        },
        function(err, rows) {
            if (err) throw err;
            for (var i = 0; i < rows.length; i++) {
                var val = i * 10 + 1;
                assert.equal(rows[i][0], 'String ' + val);
                assert.equal(rows[i][1], val);
                assert.equal(rows[i][2], val * Math.PI);
                assert.equal(rows[i][3], null);
                retrieved++;
            }
        }
    );

    beforeExit(function() {
        assert.equal(10, retrieved, "Didn't retrieve all rows");
    });
};


exports['test prepare() parameter binding'] = function(beforeExit) {
    var db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY);

    var retrieved = 0;

    db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num = ? AND txt = ?", 10, 'String 10')
      .get(function(err, row) {
        if (err) throw err;
        assert.equal(row[0], 'String 10');
        assert.equal(row[1], 10);
        assert.equal(row[2], 10 * Math.PI);
        assert.equal(row[3], null);
        retrieved++;
    });

    beforeExit(function() {
        assert.equal(1, retrieved, "Didn't retrieve all rows");
    });
};


exports['test get() parameter binding'] = function(beforeExit) {
    var db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY);

    var retrieved = 0;

    db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num = ? AND txt = ?")
      .get(10, 'String 10', function(err, row) {
        if (err) throw err;
        assert.equal(row[0], 'String 10');
        assert.equal(row[1], 10);
        assert.equal(row[2], 10 * Math.PI);
        assert.equal(row[3], null);
        retrieved++;
    });

    beforeExit(function() {
        assert.equal(1, retrieved, "Didn't retrieve all rows");
    });
};

exports['test all()'] = function(beforeExit) {
    var db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY);

    var retrieved = 0;
    var count = 1000;

    db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num < ? ORDER BY num", count)
      .all(function(err, rows) {
        if (err) throw err;
        for (var i = 0; i < rows.length; i++) {
            assert.equal(rows[i][0], 'String ' + i);
            assert.equal(rows[i][1], i);
            assert.equal(rows[i][2], i * Math.PI);
            assert.equal(rows[i][3], null);
            retrieved++;
        }
    });

    beforeExit(function() {
        assert.equal(count, retrieved, "Didn't retrieve all rows");
    });
};



exports['test all() parameter binding'] = function(beforeExit) {
    var db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY);

    var retrieved = 0;
    var count = 1000;

    db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num < ? ORDER BY num")
      .all(count, function(err, rows) {
        if (err) throw err;
        for (var i = 0; i < rows.length; i++) {
            assert.equal(rows[i][0], 'String ' + i);
            assert.equal(rows[i][1], i);
            assert.equal(rows[i][2], i * Math.PI);
            assert.equal(rows[i][3], null);
            retrieved++;
        }
    });

    beforeExit(function() {
        assert.equal(count, retrieved, "Didn't retrieve all rows");
    });
};


exports['test all() without results'] = function(beforeExit) {
    var db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY);

    var empty = false;

    db.prepare("SELECT txt, num, flt, blb FROM foo WHERE num > 5000")
      .all(function(err, rows) {
        if (err) throw err;
        assert.ok(rows.length === 0);
        empty = true;
    });

    beforeExit(function() {
        assert.ok(empty, "Didn't retrieve an empty result set");
    });
};
