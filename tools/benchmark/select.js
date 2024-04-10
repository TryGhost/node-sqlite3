const sqlite3 = require('../../');
const { readFileSync } = require('fs');


async function benchmark() {
    const db = await sqlite3.Database.create(':memory:');
    
    await db.serialize(async () => {
        await db.exec(readFileSync(`${__dirname}/select-data.sql`, 'utf8'));
        console.time('db.each');
    
        {
            const results = [];
            for await (const row of await db.each('SELECT * FROM foo')) {
                results.push(row);
            }
            console.timeEnd('db.each');
            console.time('db.all');
        }
    
        await db.all('SELECT * FROM foo');
        console.timeEnd('db.all');
    
        await db.close();
    });
}

benchmark();
