var sqlite3 = require('../../lib/sqlite3');

function randomString() {
    var str = '';
    var chars = 'abcdefghijklmnopqrstuvwxzyABCDEFGHIJKLMNOPQRSTUVWXZY0123456789  ';
    for (var i = Math.random() * 100; i > 0; i--) {
        str += chars[Math.floor(Math.random() * chars.length)];
    }
    return str;
};

var db = new sqlite3.Database('test/support/big.db');

var count = 10000000;

db.serialize(function() {
    db.run("CREATE TABLE foo (id INT, txt TEXT)");

    db.run("BEGIN TRANSACTION");
    var stmt = db.prepare("INSERT INTO foo VALUES(?, ?)");
    for (var i = 0; i < count; i++) {
        stmt.run(i, randomString());
    }
    stmt.finalize();
    db.run("COMMIT TRANSACTION");
});
