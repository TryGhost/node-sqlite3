const sqlite3 = require('..');
const assert = require('assert');

describe('data types', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
        await db.run("CREATE TABLE txt_table (txt TEXT)");
        await db.run("CREATE TABLE int_table (int INTEGER)");
        await db.run("CREATE TABLE flt_table (flt FLOAT)");
        await db.wait();
    });

    beforeEach(async function() {
        await db.exec('DELETE FROM txt_table; DELETE FROM int_table; DELETE FROM flt_table;');
    });

    it('should serialize Date()', async function() {
        const date = new Date();
        await db.run("INSERT INTO int_table VALUES(?)", date);
        const row = await db.get("SELECT int FROM int_table");
        assert.equal(row.int, +date);
    });

    it('should serialize RegExp()', async function() {
        const regexp = /^f\noo/;
        await db.run("INSERT INTO txt_table VALUES(?)", regexp);
        const row = await db.get("SELECT txt FROM txt_table");
        assert.equal(row.txt, String(regexp));
    });

    [
        4294967296.249,
        Math.PI,
        3924729304762836.5,
        new Date().valueOf(),
        912667.394828365,
        2.3948728634826374e+83,
        9.293476892934982e+300,
        Infinity,
        -9.293476892934982e+300,
        -2.3948728634826374e+83,
        -Infinity
    ].forEach(function(flt) {
        it('should serialize float ' + flt, async function() {
            await db.run("INSERT INTO flt_table VALUES(?)", flt);
            const row = await db.get("SELECT flt FROM flt_table");
            assert.equal(row.flt, flt);
        });
    });

    [
        4294967299,
        3924729304762836,
        new Date().valueOf(),
        2.3948728634826374e+83,
        9.293476892934982e+300,
        Infinity,
        -9.293476892934982e+300,
        -2.3948728634826374e+83,
        -Infinity
    ].forEach(function(integer) {
        it('should serialize integer ' + integer, async function() {
            await db.run("INSERT INTO int_table VALUES(?)", integer);
            const row = await db.get("SELECT int AS integer FROM int_table");
            assert.equal(row.integer, integer);
        });
    });

    it('should ignore faulty toString', async function() {
        const faulty = { toString: 23 };
        // TODO: This is the previous behavior, but it seems like maybe the
        // test is labelled incorrectly?
        await assert.rejects(db.run("INSERT INTO txt_table VALUES(?)", faulty));
    });

    it('should ignore faulty toString in array', async function() {
        const faulty = [[{toString: null}], 1];
        await db.all('SELECT * FROM txt_table WHERE txt = ? LIMIT ?', faulty);
    });

    it('should ignore faulty toString set to function', async function() {
        const faulty = [[{toString: function () {console.log('oh no');}}], 1];
        await db.all('SELECT * FROM txt_table WHERE txt = ? LIMIT ?', faulty);
    });

});
