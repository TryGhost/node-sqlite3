var sqlite3 = require('../lib/sqlite3');
var fs = require('fs');

var iterations = 10000;

exports.compare = {
    'insert literal file': function(finished) {
        var db = new sqlite3.Database('');
        var file = fs.readFileSync('benchmark/insert-transaction.sql', 'utf8');
        db.exec(file);
        db.close(finished)
    },

    'insert with transaction and two statements': function(finished) {
        var db = new sqlite3.Database('');

        db.serialize(function() {
            db.run("CREATE TABLE foo (id INT, txt TEXT)");
            db.run("BEGIN");

            db.parallelize(function() {
                var stmt1 = db.prepare("INSERT INTO foo VALUES (?, ?)");
                var stmt2 = db.prepare("INSERT INTO foo VALUES (?, ?)");
                for (var i = 0; i < iterations; i++) {
                    stmt1.run(i, 'Row ' + i);
                    i++;
                    stmt2.run(i, 'Row ' + i);
                }
                stmt1.finalize();
                stmt2.finalize();
            });

            db.run("COMMIT");
        });

        db.close(finished);
    },
    'insert with transaction': function(finished) {
        var db = new sqlite3.Database('');

        db.serialize(function() {
            db.run("CREATE TABLE foo (id INT, txt TEXT)");
            db.run("BEGIN");
            var stmt = db.prepare("INSERT INTO foo VALUES (?, ?)");
            for (var i = 0; i < iterations; i++) {
                stmt.run(i, 'Row ' + i);
            }
            stmt.finalize();
            db.run("COMMIT");
        });

        db.close(finished);
    },
    'insert without transaction': function(finished) {
        var db = new sqlite3.Database('');

        db.serialize(function() {
            db.run("CREATE TABLE foo (id INT, txt TEXT)");
            var stmt = db.prepare("INSERT INTO foo VALUES (?, ?)");
            for (var i = 0; i < iterations; i++) {
                stmt.run(i, 'Row ' + i);
            }
            stmt.finalize();
        });

        db.close(finished);
    }
};