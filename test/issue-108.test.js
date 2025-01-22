const sqlite3 = require('..');

describe('buffer', function() {
    /** @type {sqlite3.Database} */
    let db;
    // before(function() {
    // });

    it('should insert blobs', async function() {
        db = await sqlite3.Database.create(':memory:');
        await db.serialize(async function () {

            await db.run("CREATE TABLE lorem (info BLOB)");
            const stmt = await db.prepare("INSERT INTO lorem VALUES (?)");

            const buff = Buffer.alloc(2);
            await stmt.run(buff);
            await stmt.finalize();
        });

        await db.close();
    });
});
