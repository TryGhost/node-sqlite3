var sqlite3 = require('..');
var assert = require('assert');
var fs = require('fs');
var helper = require('./support/helper');

// Check that the number of rows in two tables matches.
function assertRowsMatchDb(db1, table1, db2, table2, done) {
    db1.get("SELECT COUNT(*) as count FROM " + table1, function(err, row) {
        if (err) throw err;
        db2.get("SELECT COUNT(*) as count FROM " + table2, function(err, row2) {
            if (err) throw err;
            assert.equal(row.count, row2.count);
            done();
        });
    });
}

// Check that the number of rows in the table "foo" is preserved in a backup.
function assertRowsMatchFile(db, backupName, done) {
    var db2 = new sqlite3.Database(backupName, sqlite3.OPEN_READONLY, function(err) {
        if (err) throw err;
        assertRowsMatchDb(db, 'foo', db2, 'foo', function() {
            db2.close(done);
        });
    });
}

describe('backup', function() {
    before(function() {
        helper.ensureExists('test/tmp');
    });

    var db;
    beforeEach(function(done) {
        helper.deleteFile('test/tmp/backup.db');
        helper.deleteFile('test/tmp/backup2.db');
        db = new sqlite3.Database('test/support/prepare.db', sqlite3.OPEN_READONLY, done);
    });

    afterEach(function(done) {
        if (!db) { done(); }
        db.close(done);
    });

    it ('output db created once step is called', function(done) {
        var backup = db.backup('test/tmp/backup.db', function(err) {
            if (err) throw err;
            backup.step(1, function(err) {
                if (err) throw err;
                assert.fileExists('test/tmp/backup.db');
                backup.finish(done);
            });
        });
    });

    it ('copies source fully with step(-1)', function(done) {
        var backup = db.backup('test/tmp/backup.db');
        backup.step(-1, function(err) {
            if (err) throw err;
            assert.fileExists('test/tmp/backup.db');
            backup.finish(function(err) {
                if (err) throw err;
                assertRowsMatchFile(db, 'test/tmp/backup.db', done);
            });
        });
    });

    it ('backup db not created if finished immediately', function(done) {
        var backup = db.backup('test/tmp/backup.db');
        backup.finish(function(err) {
            if (err) throw err;
            assert.fileDoesNotExist('test/tmp/backup.db');
            done();
        });
    });

    it ('error closing db if backup not finished', function(done) {
        var backup = db.backup('test/tmp/backup.db');
        db.close(function(err) {
            db = null;
            if (!err) throw new Error('should have an error');
            if (err.errno == sqlite3.BUSY) {
                done();
            }
            else throw err;
        });
    });

    it ('using the backup after finished is an error', function(done) {
        var backup = db.backup('test/tmp/backup.db');
        backup.finish(function(err) {
            if (err) throw err;
            backup.step(1, function(err) {
                if (!err) throw new Error('should have an error');
                if (err.errno == sqlite3.MISUSE &&
                    err.message === 'SQLITE_MISUSE: Backup is already finished') {
                    done();
                }
                else throw err;
            });
        });
    });

    it ('remaining/pageCount are available after call to step', function(done) {
        var backup = db.backup('test/tmp/backup.db');
        backup.step(0, function(err) {
            if (err) throw err;
            assert.equal(typeof this.pageCount, 'number');
            assert.equal(typeof this.remaining, 'number');
            assert.equal(this.remaining, this.pageCount);
            var prevRemaining = this.remaining;
            var prevPageCount = this.pageCount;
            backup.step(1, function(err) {
                if (err) throw err;
                assert.notEqual(this.remaining, prevRemaining);
                assert.equal(this.pageCount, prevPageCount);
                backup.finish(done);
            });
        });
    });

    it ('backup works if database is modified half-way through', function(done) {
        var backup = db.backup('test/tmp/backup.db');
        backup.step(-1, function(err) {
            if (err) throw err;
            backup.finish(function(err) {
                if (err) throw err;
                var db2 = new sqlite3.Database('test/tmp/backup.db', function(err) {
                    if (err) throw err;
                    var backup2 = db2.backup('test/tmp/backup2.db');
                    backup2.step(1, function(err, completed) {
                        if (err) throw err;
                        assert.equal(completed, false);  // Page size for the test db
                        // should not be raised to high.
                        db2.exec("insert into foo(txt) values('hello')", function(err) {
                            if (err) throw err;
                            backup2.step(-1, function(err, completed) {
                                if (err) throw err;
                                assert.equal(completed, true);
                                assertRowsMatchFile(db2, 'test/tmp/backup2.db', function() {
                                    backup2.finish(function(err) {
                                        if (err) throw err;
                                        db2.close(done);
                                    });
                                });
                            });
                        });
                    });
                });
            });
        });
    });

    (sqlite3.VERSION_NUMBER < 3026000 ? it.skip : it) ('can backup from temp to main', function(done) {
        db.exec("CREATE TEMP TABLE space (txt TEXT)", function(err) {
            if (err) throw err;
            db.exec("INSERT INTO space(txt) VALUES('monkey')", function(err) {
                if (err) throw err;
                var backup = db.backup('test/tmp/backup.db', 'temp', 'main', true, function(err) {
                    if (err) throw err;
                    backup.step(-1, function(err) {
                        if (err) throw err;
                        backup.finish(function(err) {
                            if (err) throw err;
                            var db2 = new sqlite3.Database('test/tmp/backup.db', function(err) {
                                if (err) throw err;
                                db2.get("SELECT * FROM space", function(err, row) {
                                    if (err) throw err;
                                    assert.equal(row.txt, 'monkey');
                                    db2.close(done);
                                });
                            });
                        });
                    });
                });
            });
        });
    });

    (sqlite3.VERSION_NUMBER < 3026000 ? it.skip : it) ('can backup from main to temp', function(done) {
        var backup = db.backup('test/support/prepare.db', 'main', 'temp', false, function(err) {
            if (err) throw err;
            backup.step(-1, function(err) {
                if (err) throw err;
                backup.finish(function(err) {
                    if (err) throw err;
                    assertRowsMatchDb(db, 'temp.foo', db, 'main.foo', done);
                });
            });
        });
    });

    it ('cannot backup to a locked db', function(done) {
        var db2 = new sqlite3.Database('test/tmp/backup.db', function(err) {
            db2.exec("PRAGMA locking_mode = EXCLUSIVE");
            db2.exec("BEGIN EXCLUSIVE", function(err) {
                if (err) throw err;
                var backup = db.backup('test/tmp/backup.db');
                backup.step(-1, function(stepErr) {
                    db2.close(function(err) {
                        if (err) throw err;
                        if (stepErr.errno == sqlite3.BUSY) {
                            backup.finish(done);
                        }
                        else throw stepErr;
                    });
                });
            });
        });
    });

    it ('fuss-free incremental backups work', function(done) {
        var backup = db.backup('test/tmp/backup.db');
        var timer;
        function makeProgress() {
            if (backup.idle) {
                backup.step(1);
            }
            if (backup.completed || backup.failed) {
                clearInterval(timer);
                assert.equal(backup.completed, true);
                assert.equal(backup.failed, false);
                done();
            }
        }
        timer = setInterval(makeProgress, 2);
    });

    it ('setting retryErrors to empty disables automatic finishing', function(done) {
        var backup = db.backup('test/tmp/backup.db');
        backup.retryErrors = [];
        backup.step(-1, function(err) {
            if (err) throw err;
            db.close(function(err) {
                db = null;
                if (!err) throw new Error('should have an error');
                assert.equal(err.errno, sqlite3.BUSY);
                done();
            });
        });
    });

    it ('setting retryErrors enables automatic finishing', function(done) {
        var backup = db.backup('test/tmp/backup.db');
        backup.retryErrors = [sqlite3.OK];
        backup.step(-1, function(err) {
            if (err) throw err;
            db.close(function(err) {
                if (err) throw err;
                db = null;
                done();
            });
        });
    });

    it ('default retryErrors will retry on a locked/busy db', function(done) {
        var db2 = new sqlite3.Database('test/tmp/backup.db', function(err) {
            db2.exec("PRAGMA locking_mode = EXCLUSIVE");
            db2.exec("BEGIN EXCLUSIVE", function(err) {
                if (err) throw err;
                var backup = db.backup('test/tmp/backup.db');
                backup.step(-1, function(stepErr) {
                    db2.close(function(err) {
                        if (err) throw err;
                        assert.equal(stepErr.errno, sqlite3.BUSY);
                        assert.equal(backup.completed, false);
                        assert.equal(backup.failed, false);
                        backup.step(-1, function(err) {
                            if (err) throw err;
                            assert.equal(backup.completed, true);
                            assert.equal(backup.failed, false);
                            done();
                        });
                    });
                });
            });
        });
    });
});
