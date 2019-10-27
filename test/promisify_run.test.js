var sqlite3 = require('..');
var assert = require('assert');
var util = require('util');

if (typeof util.promisify === 'function') {
    describe('promisify run', function() {
        var db;
        var promisifyedDbRun;
        before(function(done) {
            db = new sqlite3.Database(':memory:', done);
            promisifyedDbRun = util.promisify(db.run).bind(db);
        });

        it('should create the table', function(done) {
            promisifyedDbRun("CREATE TABLE foo (txt TEXT, num INT)")
                .then(function(result) {
                    assert.deepEqual(result, {
                        changes: 0,
                        lastID: 0,
                        sql: 'CREATE TABLE foo (txt TEXT, num INT)'
                    });
                    done();
                })
                .catch(done);
        });

        it('should insert a value without placeholders', function(done) {
            promisifyedDbRun("INSERT INTO foo VALUES('Lorem Ipsum', 1)")
                .then(function(result) {
                    assert.deepEqual(result, {
                        changes: 1,
                        lastID: 1,
                        sql: "INSERT INTO foo VALUES('Lorem Ipsum', 1)"
                    });
                    done();
                })
                .catch(done);
        });

        it('should update a value with placeholders', function(done) {
            promisifyedDbRun("UPDATE foo SET txt = $text WHERE num = $id", {
                $id: 1,
                $text: "Dolor Sit Amet"
            })
                .then(function(result) {
                    assert.deepEqual(result, {
                        changes: 1,
                        lastID: 1,
                        sql: 'UPDATE foo SET txt = $text WHERE num = $id'
                    });
                    done();
                })
                .catch(done);
        });

        it('should retrieve values', function(done) {
            db.all("SELECT txt, num FROM foo", function(err, rows) {
                if (err) throw err;
                assert.equal(rows[0].txt, "Dolor Sit Amet");
                assert.equal(rows[0].num, 1);
                assert.equal(rows.length, 1);
                done();
            });
        });
    });
}
