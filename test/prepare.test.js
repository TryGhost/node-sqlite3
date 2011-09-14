var sqlite3 = require('sqlite3');
var assert = require('assert');
var Step = require('step');

if (process.setMaxListeners) process.setMaxListeners(0);

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
            var stmt = db.prepare("SELECT txt, num, flt, blb FROM foo ORDER BY num", function(err) {
                if (err) throw err;
                assert.equal(stmt.sql, 'SELECT txt, num, flt, blb FROM foo ORDER BY num');
            });

            var group = this.group();
            for (var i = 0; i < count + 5; i++) {
                stmt.get(group());
            }
        },
        function(err, rows) {
            if (err) throw err;
            assert.equal(count + 5, rows.length, "Didn't run all queries");

            for (var i = 0; i < count; i++) {
                assert.equal(rows[i].txt, 'String ' + i);
                assert.equal(rows[i].num, i);
                assert.equal(rows[i].flt, i * Math.PI);
                assert.equal(rows[i].blb, null);
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
                assert.equal(rows[i].txt, 'String 0');
                assert.equal(rows[i].num, 0);
                assert.equal(rows[i].flt, 0.0);
                assert.equal(rows[i].blb, null);
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
                assert.equal(rows[i].txt, 'String ' + val);
                assert.equal(rows[i].num, val);
                assert.equal(rows[i].flt, val * Math.PI);
                assert.equal(rows[i].blb, null);
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
        assert.equal(row.txt, 'String 10');
        assert.equal(row.num, 10);
        assert.equal(row.flt, 10 * Math.PI);
        assert.equal(row.blb, null);
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
        assert.equal(row.txt, 'String 10');
        assert.equal(row.num, 10);
        assert.equal(row.flt, 10 * Math.PI);
        assert.equal(row.blb, null);
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
            assert.equal(rows[i].txt, 'String ' + i);
            assert.equal(rows[i].num, i);
            assert.equal(rows[i].flt, i * Math.PI);
            assert.equal(rows[i].blb, null);
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
            assert.equal(rows[i].txt, 'String ' + i);
            assert.equal(rows[i].num, i);
            assert.equal(rows[i].flt, i * Math.PI);
            assert.equal(rows[i].blb, null);
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

exports['test high concurrency'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    function randomString() {
        var str = '';
        for (var i = Math.random() * 300; i > 0; i--) {
            str += String.fromCharCode(Math.floor(Math.random() * 256));
        }
        return str;
    };

    // Generate random data.
    var data = [];
    var length = Math.floor(Math.random() * 1000) + 200;
    for (var i = 0; i < length; i++) {
        data.push([ randomString(), i, i * Math.random(), null ]);
    }

    var inserted = false;
    var retrieved = 0;

    Step(
        function() {
            db.prepare("CREATE TABLE foo (txt text, num int, flt float, blb blob)").run(this);
        },
        function(err) {
            if (err) throw err;
            var group = this.group();
            for (var i = 0; i < data.length; i++) {
                var stmt = db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)");
                stmt.run(data[i][0], data[i][1], data[i][2], data[i][3], group());
            }
        },
        function(err, result) {
            if (err) throw err;
            assert.ok(result.length === length, 'Invalid length');
            inserted = true;

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
            });
        }
    );

    beforeExit(function() {
        assert.ok(inserted);
    });
};

exports['test Database#get()'] = function(beforeExit) {
    var db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY);

    var retrieved = 0;

    db.get("SELECT txt, num, flt, blb FROM foo WHERE num = ? AND txt = ?", 10, 'String 10', function(err, row) {
        if (err) throw err;
        assert.equal(row.txt, 'String 10');
        assert.equal(row.num, 10);
        assert.equal(row.flt, 10 * Math.PI);
        assert.equal(row.blb, null);
        retrieved++;
    });

    beforeExit(function() {
        assert.equal(1, retrieved, "Didn't retrieve all rows");
    });
};

exports['test Database#run() and Database#all()'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var inserted = 0;
    var retrieved = 0;

    // We insert and retrieve that many rows.
    var count = 1000;

    Step(
        function() {
            db.run("CREATE TABLE foo (txt text, num int, flt float, blb blob)", this);
        },
        function(err) {
            if (err) throw err;
            var group = this.group();
            for (var i = 0; i < count; i++) {
                db.run("INSERT INTO foo VALUES(?, ?, ?, ?)",
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
            db.all("SELECT txt, num, flt, blb FROM foo ORDER BY num", this)
        },
        function(err, rows) {
            if (err) throw err;
            assert.equal(count, rows.length, "Couldn't retrieve all rows");

            for (var i = 0; i < count; i++) {
                assert.equal(rows[i].txt, 'String ' + i);
                assert.equal(rows[i].num, i);
                assert.equal(rows[i].flt, i * Math.PI);
                assert.equal(rows[i].blb, null);
                retrieved++;
            }
        }
    );

    beforeExit(function() {
        assert.equal(count, inserted, "Didn't insert all rows");
        assert.equal(count, retrieved, "Didn't retrieve all rows");
    });
};
