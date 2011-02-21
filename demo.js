var sqlite3 = require('./lib/sqlite3');

var db = new sqlite3.Database(':memory:');
db.prepare("sdf", function() {});
db.prepare("CREATE TABLE foo (text bar)").run().finalize(function() {
    db.prepare("INSERT INTO foo VALUES('bar baz boo')").run().finalize(function() {
        db.prepare("SELECT * FROM foo").run(function(err, row) {
            console.log(row);
        }).run(function(err, row) {
            console.log(row);
        });
    });
});
