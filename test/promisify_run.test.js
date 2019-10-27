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

        it('should create the table', function() {
            return promisifyedDbRun("CREATE TABLE foo (txt TEXT, num INT)")
                .then(function(result) {
                    assert.equal(result.changes, 0);
                    assert.equal(result.lastID, 0);
                });
        });

        it('should insert a value without placeholders', function() {
            return promisifyedDbRun("INSERT INTO foo VALUES('Lorem Ipsum', 1)")
                .then(function(result) {
                    assert.equal(result.changes, 1);
                    assert.equal(result.lastID, 1);
                });
        });

        it('should update a value with placeholders', function() {
            return promisifyedDbRun("UPDATE foo SET txt = $text WHERE num = $id", {
                $id: 1,
                $text: "Dolor Sit Amet"
            })
                .then(function(result) {
                    assert.equal(result.changes, 1);
                    assert.equal(result.lastID, 1);
                });
        });

        it('should also work with statement', function() {
            var stmt = db.prepare("INSERT INTO foo VALUES($text, $id)");
            var promisifyedStatementRun = util.promisify(stmt.run).bind(stmt);
            return promisifyedStatementRun({
                $id: 2,
                $text: "Consectetur Adipiscing Elit"
            })
                .then(function(result) {
                    assert.equal(result.changes, 1);
                    assert.equal(result.lastID, 2);
                });
        });

        it('should retrieve values', function(done) {
            db.all("SELECT txt, num FROM foo", function(err, rows) {
                if (err) throw err;
                assert.equal(rows[0].txt, "Dolor Sit Amet");
                assert.equal(rows[0].num, 1);
                assert.equal(rows[1].txt, "Consectetur Adipiscing Elit");
                assert.equal(rows[1].num, 2);
                assert.equal(rows.length, 2);
                done();
            });
        });
    });
}
