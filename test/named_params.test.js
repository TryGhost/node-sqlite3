const sqlite3 = require('..');
const assert = require('assert');

describe('named parameters', function() {
    /** @type {sqlite3.Database} */
    let db;
    before(async function() {
        db = await sqlite3.Database.create(':memory:');
    });

    it('should create the table', async function() {
        await db.run("CREATE TABLE foo (txt TEXT, num INT)");
    });

    it('should insert a value with $ placeholders', async function() {
        await db.run("INSERT INTO foo VALUES($text, $id)", {
            $id: 1,
            $text: "Lorem Ipsum"
        });
    });

    it('should insert a value with : placeholders', async function() {
        await db.run("INSERT INTO foo VALUES(:text, :id)", {
            ':id': 2,
            ':text': "Dolor Sit Amet"
        });
    });

    it('should insert a value with @ placeholders', async function() {
        await db.run("INSERT INTO foo VALUES(@txt, @id)", {
            "@id": 3,
            "@txt": "Consectetur Adipiscing Elit"
        });
    });

    it('should insert a value with @ placeholders using an array', async function() {
        await db.run("INSERT INTO foo VALUES(@txt, @id)", [ 'Sed Do Eiusmod', 4 ]);
    });

    it('should insert a value with indexed placeholders', async function() {
        await db.run("INSERT INTO foo VALUES(?2, ?4)",
            [ null, 'Tempor Incididunt', null, 5 ]);
    });

    it('should insert a value with autoindexed placeholders', async function() {
        await db.run("INSERT INTO foo VALUES(?, ?)", {
            2: 6,
            1: "Ut Labore Et Dolore"
        });
    });

    it('should retrieve all inserted values', async function() {
        const rows = await db.all("SELECT txt, num FROM foo ORDER BY num");
        assert.equal(rows[0].txt, "Lorem Ipsum");
        assert.equal(rows[0].num, 1);
        assert.equal(rows[1].txt, "Dolor Sit Amet");
        assert.equal(rows[1].num, 2);
        assert.equal(rows[2].txt, "Consectetur Adipiscing Elit");
        assert.equal(rows[2].num, 3);
        assert.equal(rows[3].txt, "Sed Do Eiusmod");
        assert.equal(rows[3].num, 4);
        assert.equal(rows[4].txt, "Tempor Incididunt");
        assert.equal(rows[4].num, 5);
        assert.equal(rows[5].txt, "Ut Labore Et Dolore");
        assert.equal(rows[5].num, 6);
    });

    after(async function() {
        await db.close();
    });
});
