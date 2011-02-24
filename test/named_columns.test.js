var sqlite3 = require('sqlite3');
var assert = require('assert');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test named columns'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var finished = false;

    db.serialize(function() {
        db.run("CREATE TABLE foo (txt TEXT, num INT)");

        db.run("INSERT INTO foo VALUES($text, $id)", {
            $id: 1,
            $text: "Lorem Ipsum"
        });

        db.get("SELECT txt, num FROM foo ORDER BY num", function(err, row) {
            if (err) throw err;

            assert.equal(row.txt, "Lorem Ipsum");
            assert.equal(row.num, 1);

            finished = true;
        });

    });

    beforeExit(function() {
        assert.ok(finished);
    });
};