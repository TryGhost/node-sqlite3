var sqlite3 = require('../lib/sqlite3');
var assert = require('assert');

describe('expandedSql', function() {
    var db;
    before(function(done) {
        db = new sqlite3.Database(':memory:');
        db.run("CREATE TABLE foo (id INT PRIMARY KEY, count INT)", done);
    });
    after(function(done) {
        db.wait(done);
    });

    it('should expand the sql with bindings within callbacks for Statement', function() {
        db.serialize(() => {
            /** test Statement methods */
            let stmt = db.prepare("INSERT INTO foo VALUES(?,?)", function() {
                assert.equal(this.expandedSql, "INSERT INTO foo VALUES(NULL,NULL)");
                return this;
            });
            /** bind */
            stmt.bind(3,4, function() {
                assert.equal(this.expandedSql, "INSERT INTO foo VALUES(3,4)");
            });
            stmt.bind(1,2, function() {
                assert.equal(this.expandedSql, "INSERT INTO foo VALUES(1,2)");
            });
            /** reset */
            stmt.reset(function() {
                // currently it is being retained
                assert.equal(this.expandedSql, "INSERT INTO foo VALUES(1,2)");
            });

            // only a utility for callbacks
            assert.equal(stmt.expandedSql, undefined);

            stmt = db.prepare("INSERT INTO foo VALUES(?,?)", function() {
                assert.equal(this.expandedSql, "INSERT INTO foo VALUES(NULL,NULL)");
                return this;
            });
            /** run */
            stmt.run([2,2], function() {
                assert.equal(this.expandedSql, "INSERT INTO foo VALUES(2,2)");
            });
            stmt.run(4,5, function() {
                assert.equal(this.expandedSql, "INSERT INTO foo VALUES(4,5)");
            });
            stmt.finalize();
            
            /** get */
            stmt = db.prepare("select * from foo where id = $id;");
            stmt.get(function() {
                assert.equal(this.expandedSql, "select * from foo where id = NULL;");
            });
            stmt.get({ $id: 1 }, function() {
                assert.equal(this.expandedSql, "select * from foo where id = 1;");
            });
            stmt.get([2], function() {
                assert.equal(this.expandedSql, "select * from foo where id = 2;");
            });

            /** all */
            stmt.all(1, function() {
                assert.equal(this.expandedSql, "select * from foo where id = 1;");
            });
            stmt.all({ $id: 2 }, function() {
                assert.equal(this.expandedSql, "select * from foo where id = 2;");
            });
            stmt.all([3], function() {
                assert.equal(this.expandedSql, "select * from foo where id = 3;");
            });

            /** each - testing within each callback */
            stmt.each(1, function() {
                assert.equal(this.expandedSql, "select * from foo where id = 1;");
            }, function() {
                assert.equal(this.expandedSql, "select * from foo where id = 1;");
            });
            stmt.each({ $id: 2 }, function() {
                assert.equal(this.expandedSql, "select * from foo where id = 2;");
            }, function() {
                assert.equal(this.expandedSql, "select * from foo where id = 2;");
            });
            stmt.each([3], function() {
                assert.equal(this.expandedSql, "select * from foo where id = 3;");
            }, function() {
                assert.equal(this.expandedSql, "select * from foo where id = 3;");
            });

            stmt.finalize();
        });
    });

    it('should expand the sql with bindings within callbacks for Database', function() {
        db.serialize(() => {
            db.prepare("INSERT INTO foo VALUES(?,?)", function() {
                assert.equal(this.expandedSql, "INSERT INTO foo VALUES(NULL,NULL)");
            });

            /** run */
            db.run("INSERT INTO foo VALUES(?,?)", 10, 11, function () {
                assert.equal(this.expandedSql, "INSERT INTO foo VALUES(10,11)");
            });

            /** get */
            db.get("select * from foo;", function() {
                assert.equal(this.expandedSql, "select * from foo;");
            });
            db.get("select * from bar;", function() {
                // since this is a sqlite error there is nothing to expand afterwards
                assert.equal(this.expandedSql, undefined);
            });

            /** all */
            db.all("SELECT * FROM foo;", function(err, rows) {
                assert.equal(this.expandedSql, "SELECT * FROM foo;");
                assert.deepStrictEqual(rows.sort((a,b) => a.id - b.id), [{ id: 2, count: 2 }, { id: 4, count: 5}, { id: 10, count: 11 }]);
            });
            db.all("SELECT id, count FROM foo WHERE id = ?", 1, function(err, rows) {
                assert.equal(this.expandedSql, "SELECT id, count FROM foo WHERE id = 1");
                assert.deepStrictEqual(rows, []);
            });

            /** each */
            db.each("select * from foo;", function() {
                assert.equal(this.expandedSql, "select * from foo;");
            }, function() {
                assert.equal(this.expandedSql, "select * from foo;");
            });
            db.each("select * from bar;", function() {
                // since this is a sqlite error there is nothing to expand afterwards
                assert.equal(this.expandedSql, undefined);
            });

            /** exec */
            db.exec("select * from foo where id = ?", function() {
                // not applicable here so it is never set
                assert.equal(this.expandedSql, undefined);
            });
        });
    });
});
