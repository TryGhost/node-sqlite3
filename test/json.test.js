const sqlite3 = require('..');

if( process.env.NODE_SQLITE3_JSON1 === 'no' ){
    describe('json', function() {
        it(
            'skips JSON tests when --sqlite=/usr (or similar) is tested',
            function(){}
        );
    });
} else {
    describe('json', function() {
        /** @type {sqlite3.Database} */
        let db;

        before(async function() {
            db = await sqlite3.Database.create(':memory:');
        });

        it('should select JSON', async function() {
            await db.run('SELECT json(?)', JSON.stringify({ok:true}));
        });
    });
}
