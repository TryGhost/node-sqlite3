#!/usr/bin/env node

async function createdb() {
    var existsSync = require('fs').existsSync || require('path').existsSync;
    var statSync   = require('fs').statSync || require('path').statSync;
    var path = require('path');

    var sqlite3 = require('../../lib/sqlite3');

    var count = 1000000;
    var db_path = path.join(__dirname,'big.db');

    function randomString() {
        var str = '';
        var chars = 'abcdefghijklmnopqrstuvwxzyABCDEFGHIJKLMNOPQRSTUVWXZY0123456789  ';
        for (var i = Math.random() * 100; i > 0; i--) {
            str += chars[Math.floor(Math.random() * chars.length)];
        }
        return str;
    }

    // Make sure the file exists and is also valid.
    if (existsSync(db_path) && statSync(db_path).size !== 0) {
        console.log('okay: database already created (' + db_path + ')');
    } else {
        console.log("Creating test database... This may take several minutes.");
        var db = await sqlite3.Database.create(db_path);
        await db.serialize(async function() {
            await db.run("CREATE TABLE foo (id INT, txt TEXT)");
            db.run("BEGIN TRANSACTION");
            var stmt = await db.prepare("INSERT INTO foo VALUES(?, ?)");
            for (var i = 0; i < count; i++) {
                stmt.run(i, randomString());
            }
            await stmt.finalize();
            await db.run("COMMIT TRANSACTION", []);
            await db.close();
        });
    }
}

if (require.main === module) {
    createdb();
}

module.exports = createdb;
