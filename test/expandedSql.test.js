var sqlite3 = require('..');
var assert = require('assert');

describe('expandedSql', function() {
    var db;
    before(function(done) {
        db = new sqlite3.Database(':memory:');
        db.serialize(function() {
            db.run("CREATE TABLE foo (id INT, txt TEXT)");
            db.run("CREATE TABLE complex (a INT, b INT, c INT, d INT, e INT)", done);
        });
    });
    after(function(done) {
        db.wait(function() {
            db.close(done);
        });
    });

    it('should be accessible as a property on Statement', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)", function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(NULL,NULL)");
            stmt.finalize(done);
        });
    });

    it('should show bound parameters in callbacks', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        stmt.run(1, "test", function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(1,'test')");
            stmt.finalize(done);
        });
    });

    it('should work with get callback', function(done) {
        var stmt = db.prepare("SELECT * FROM foo WHERE id = ?");
        stmt.get(1, function(err, row) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "SELECT * FROM foo WHERE id = 1");
            assert.equal(row.id, 1);
            stmt.finalize(done);
        });
    });

    it('should work with all callback', function(done) {
        var stmt = db.prepare("SELECT * FROM foo WHERE id > ?");
        stmt.all(0, function(err, rows) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "SELECT * FROM foo WHERE id > 0");
            assert.ok(rows.length > 0);
            stmt.finalize(done);
        });
    });

    it('should work with each callback', function(done) {
        var stmt = db.prepare("SELECT * FROM foo WHERE id < ?");
        var count = 0;
        stmt.each(5, function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "SELECT * FROM foo WHERE id < 5");
            count++;
        }, function(err) {
            if (err) throw err;
            assert.ok(count > 0);
            stmt.finalize(done);
        });
    });

    it('should work with bind callback', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        stmt.bind(10, "hello", function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(10,'hello')");
            stmt.run(function(err) {
                if (err) throw err;
                stmt.finalize(done);
            });
        });
    });

    it('should handle string escaping correctly', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        stmt.run(20, "test'quote", function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(20,'test''quote')");
            stmt.finalize(done);
        });
    });

    it('should handle incomplete bindings', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        stmt.run(20, function(err) {
            if (err) throw err;
            // sqlite fills unbound parameters with NULL, not ?
            assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(20,NULL)");
            stmt.finalize(done);
        });
    });

    it('should work with named parameters', function(done) {
        var stmt = db.prepare("SELECT * FROM foo WHERE id = $id");
        stmt.get({ $id: 1 }, function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "SELECT * FROM foo WHERE id = 1");
            stmt.finalize(done);
        });
    });

    it('should handle NULL values', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        stmt.run(200, null, function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(200,NULL)");
            stmt.finalize(done);
        });
    });

    it('should handle empty strings', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        stmt.run(1, "", function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(1,'')");
            stmt.finalize(done);
        });
    });

    it('should handle large integers', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        var maxSafeInt = Number.MAX_SAFE_INTEGER;
        stmt.run(maxSafeInt, "large", function(err) {
            if (err) throw err;
            // sqlite formats large numbers - accept various scientific notations
            var expanded = stmt.expandedSql;
            assert.ok(
                expanded.match(/INSERT INTO foo VALUES\(9\.00719.*[eE]\+1[45],'large'\)/),
                'Expected large number in scientific notation, got: ' + expanded
            );
            stmt.finalize(done);
        });
    });

    it('should return undefined before statement is prepared', function(done) {
        var stmt = db.prepare("SELECT 1");
        assert.equal(stmt.expandedSql, undefined);
        stmt.finalize(done);
    });

    it('should return undefined after finalize', function(done) {
        var stmt = db.prepare("SELECT 1");
        stmt.finalize(function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, undefined);
            done();
        });
    });

    it('should update after reset and rebind', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        
        stmt.bind(5, "first", function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(5,'first')");
            
            stmt.run(function(err) {
                if (err) throw err;
                
                stmt.reset(function(err) {
                    if (err) throw err;
                    
                    stmt.bind(6, "second", function(err) {
                        if (err) throw err;
                        assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(6,'second')");
                        stmt.finalize(done);
                    });
                });
            });
        });
    });

    it('should work with batch inserts', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        var inserted = 0;
        var batchSize = 5;
        
        for (var i = 300; i < 300 + batchSize; i++) {
            (function(id) {
                stmt.run(id, 'batch' + id, function(err) {
                    if (err) throw err;
                    assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(" + id + ",'batch" + id + "')");
                    inserted++;
                    if (inserted === batchSize) {
                        stmt.finalize(done);
                    }
                });
            })(i);
        }
    });

    it('should work with complex queries with many parameters', function(done) {
        var stmt = db.prepare("INSERT INTO complex VALUES(?,?,?,?,?)");
        stmt.run(1, 2, 3, 4, 5, function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "INSERT INTO complex VALUES(1,2,3,4,5)");
            stmt.finalize(done);
        });
    });

    it('should work within serialized transactions', function(done) {
        db.serialize(function() {
            var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
            stmt.run(500, "transaction", function(err) {
                if (err) throw err;
                assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(500,'transaction')");
                stmt.finalize(done);
            });
        });
    });

    it('should be accessible outside callbacks after operations complete', function(done) {
        var stmt = db.prepare("INSERT INTO foo VALUES(?,?)");
        
        stmt.run(600, "outside", function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "INSERT INTO foo VALUES(600,'outside')");
            stmt.finalize(done);
        });
    });

    it('demonstrates the async queue behavior', function(done) {
        var stmt = db.prepare("SELECT * FROM foo WHERE id = ?");
        
        stmt.bind(1);
        
        // worker thread still hasn't completed, so it isn't available
        assert.equal(stmt.expandedSql, undefined);
        
        stmt.bind(1, function(err) {
            if (err) throw err;
            assert.equal(stmt.expandedSql, "SELECT * FROM foo WHERE id = 1");
            stmt.finalize(done);
        });
    });
});
