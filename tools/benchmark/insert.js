const sqlite3 = require('../../lib/sqlite3');
const fs = require('fs');

const iterations = 10000;

exports.compare = {
    'insert literal file': async function(finished) {
        const db = await sqlite3.Database.create('');
        const file = fs.readFileSync('benchmark/insert-transaction.sql', 'utf8');
        await db.exec(file);
        await db.close();
        finished();
    },

    'insert with transaction and two statements': async function(finished) {
        const db = await sqlite3.Database.create('');

        await db.serialize(async function() {
            await db.run("CREATE TABLE foo (id INT, txt TEXT)");
            await db.run("BEGIN");

            await db.parallelize(async function() {
                const stmt1 = await db.prepare("INSERT INTO foo VALUES (?, ?)");
                const stmt2 = await db.prepare("INSERT INTO foo VALUES (?, ?)");
                for (let i = 0; i < iterations; i++) {
                    await stmt1.run(i, 'Row ' + i);
                    i++;
                    await stmt2.run(i, 'Row ' + i);
                }
                await stmt1.finalize();
                await stmt2.finalize();
            });

            await db.run("COMMIT");
        });

        await db.close();
        finished();
    },
    'insert with transaction': async function(finished) {
        const db = await sqlite3.Database.create('');

        await db.serialize(async function() {
            await db.run("CREATE TABLE foo (id INT, txt TEXT)");
            await db.run("BEGIN");
            const stmt = await db.prepare("INSERT INTO foo VALUES (?, ?)");
            for (let i = 0; i < iterations; i++) {
                await stmt.run(i, 'Row ' + i);
            }
            await stmt.finalize();
            await db.run("COMMIT");
        });

        await db.close();
        finished();
    },
    'insert without transaction': async function(finished) {
        const db = await sqlite3.Database.create('');

        await db.serialize(async function() {
            await db.run("CREATE TABLE foo (id INT, txt TEXT)");
            const stmt = await db.prepare("INSERT INTO foo VALUES (?, ?)");
            for (let i = 0; i < iterations; i++) {
                await stmt.run(i, 'Row ' + i);
            }
            await stmt.finalize();
        });

        await db.close();
        finished();
    }
};
