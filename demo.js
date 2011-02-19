var sqlite3 = require('./lib/sqlite3');

var db = new sqlite3.Database(':memory:');
db.prepare("CREATE TABLE foo (text bar)")
    .run(function(err) {
        console.log('x');
    })
    .finalize()
.close();
