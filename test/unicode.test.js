const sqlite3 = require('..');
const assert = require('assert');

describe('unicode', function() {
    const first_values = [];
    const trailing_values = [];
    const subranges = new Array(2);
    const len = subranges.length;
    let db;
    let i;

    before(async function() {
        db = await sqlite3.Database.create(':memory:');
    });

    for (i = 0x20; i < 0x80; i++) {
        first_values.push(i);
    }

    for (i = 0xc2; i < 0xf0; i++) {
        first_values.push(i);
    }

    for (i = 0x80; i < 0xc0; i++) {
        trailing_values.push(i);
    }

    for (i = 0; i < len; i++) {
        subranges[i] = [];
    }

    for (i = 0xa0; i < 0xc0; i++) {
        subranges[0].push(i);
    }

    for (i = 0x80; i < 0xa0; i++) {
        subranges[1].push(i);
    }

    function random_choice(arr) {
        return arr[Math.random() * arr.length | 0];
    }

    function random_utf8() {
        const first = random_choice(first_values);

        if (first < 0x80) {
            return String.fromCharCode(first);
        } else if (first < 0xe0) {
            return String.fromCharCode((first & 0x1f) << 0x6 | random_choice(trailing_values) & 0x3f);
        } else if (first == 0xe0) {
            return String.fromCharCode(((first & 0xf) << 0xc) | ((random_choice(subranges[0]) & 0x3f) << 6) | random_choice(trailing_values) & 0x3f);
        } else if (first == 0xed) {
            return String.fromCharCode(((first & 0xf) << 0xc) | ((random_choice(subranges[1]) & 0x3f) << 6) | random_choice(trailing_values) & 0x3f);
        } else if (first < 0xf0) {
            return String.fromCharCode(((first & 0xf) << 0xc) | ((random_choice(trailing_values) & 0x3f) << 6) | random_choice(trailing_values) & 0x3f);
        }
    }

    function randomString() {
        let str = '';
        let i;

        for (i = Math.random() * 300; i > 0; i--) {
            str += random_utf8();
        }

        return str;
    }


    // Generate random data.
    const data = [];
    const length = Math.floor(Math.random() * 1000) + 200;
    for (let i = 0; i < length; i++) {
        data.push(randomString());
    }

    let inserted = 0;
    let retrieved = 0;
    
    it('should retrieve all values', async function() {
        await db.run("CREATE TABLE foo (id int, txt text)");
        const stmt = await db.prepare("INSERT INTO foo VALUES(?, ?)");
        for (let i = 0; i < data.length; i++) {
            await stmt.run(i, data[i]);
            inserted++;
        }
        await stmt.finalize();

        const rows = await db.all("SELECT txt FROM foo ORDER BY id");

        for (let i = 0; i < rows.length; i++) {
            assert.equal(rows[i].txt, data[i]);
            retrieved++;
        }
        assert.equal(inserted, length);
        assert.equal(retrieved, length);
    });

    after(async function() {
        await db.close();
    });
});
