#!/usr/bin/env node

async function createdb() {
    const existsSync = require('fs').existsSync || require('path').existsSync;
    const statSync   = require('fs').statSync || require('path').statSync;
    const path = require('path');

    const sqlite3 = require('../../lib/sqlite3');

    const count = 1000000;
    const dbPath = path.join(__dirname,'big.db');

    function randomString() {
        let str = '';
        const chars = 'abcdefghijklmnopqrstuvwxzyABCDEFGHIJKLMNOPQRSTUVWXZY0123456789  ';
        for (let i = Math.random() * 100; i > 0; i--) {
            str += chars[Math.floor(Math.random() * chars.length)];
        }
        return str;
    }

    // Make sure the file exists and is also valid.
    if (existsSync(dbPath) && statSync(dbPath).size !== 0) {
        console.log('okay: database already created (' + dbPath + ')');
    } else {
        console.log("Creating test database... This may take several minutes.");
        const db = await sqlite3.Database.create(dbPath);
        await db.serialize(async function() {
            await db.run("CREATE TABLE foo (id INT, txt TEXT)");
            await db.run("BEGIN TRANSACTION");
            const stmt = await db.prepare("INSERT INTO foo VALUES(?, ?)");
            for (let i = 0; i < count; i++) {
                await stmt.run(i, randomString());
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
