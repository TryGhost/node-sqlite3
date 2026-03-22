const sqlite3 = require('../../');
const { readFileSync } = require('fs');

exports.compare = {
    'db.each': function (finished) {
        const db = new sqlite3.Database(':memory:');

        // Prepare the database first
        db.serialize(() => {
            db.exec(readFileSync(`${__dirname}/select-data.sql`, 'utf8'));

            const results = [];
            db.each('SELECT * FROM foo', (err, row) => {
                if (err) throw err;
                results.push(row);
            }, () => {
                db.close(finished);
            });
        });
    },
    'db.all': function (finished) {
        const db = new sqlite3.Database(':memory:');

        // Prepare the database first
        db.serialize(() => {
            db.exec(readFileSync(`${__dirname}/select-data.sql`, 'utf8'));

            db.all('SELECT * FROM foo', (err, rows) => {
                if (err) throw err;
                db.close(finished);
            });
        });
    },
    'db.all with statement reset': function (finished) {
        const db = new sqlite3.Database(':memory:');

        // Prepare the database first
        db.serialize(() => {
            db.exec(readFileSync(`${__dirname}/select-data.sql`, 'utf8'));

            const stmt = db.prepare('SELECT * FROM foo');
            stmt.all((err, rows) => {
                if (err) throw err;
                stmt.reset();
                stmt.all((err2, rows2) => {
                    if (err2) throw err2;
                    stmt.finalize();
                    db.close(finished);
                });
            });
        });
    }
};
