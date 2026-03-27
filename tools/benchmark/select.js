const sqlite3 = require('../../');
const { readFileSync } = require('fs');

const setupSQL = readFileSync(`${__dirname}/select-data.sql`, 'utf8');

/**
 * Helper to promisify sqlite3 operations
 */
function promisifyExec(db, sql) {
    return new Promise((resolve, reject) => {
        db.exec(sql, (err) => {
            if (err) reject(err);
            else resolve();
        });
    });
}

function promisifyClose(db) {
    return new Promise((resolve, reject) => {
        db.close((err) => {
            if (err) reject(err);
            else resolve();
        });
    });
}

/**
 * Select benchmarks with proper setup/teardown separation.
 * Setup (create database, populate data) and teardown (close database)
 * are NOT measured - only the actual select operations are measured.
 */
module.exports = {
    benchmarks: {
        'db.each': {
            async beforeEach() {
                // Setup: Create and populate database - NOT measured
                this.db = new sqlite3.Database(':memory:');
                await promisifyExec(this.db, setupSQL);
            },

            async fn() {
                // Benchmark: Iterate through results row by row - MEASURED
                const db = this.db;
                await new Promise((resolve, reject) => {
                    const results = [];
                    db.each('SELECT * FROM foo', (err, row) => {
                        if (err) reject(err);
                        results.push(row);
                    }, (err) => {
                        if (err) reject(err);
                        else resolve();
                    });
                });
            },

            async afterEach() {
                // Teardown: Close database - NOT measured
                await promisifyClose(this.db);
            },
        },

        'db.all': {
            async beforeEach() {
                // Setup: Create and populate database - NOT measured
                this.db = new sqlite3.Database(':memory:');
                await promisifyExec(this.db, setupSQL);
            },

            async fn() {
                // Benchmark: Retrieve all results at once - MEASURED
                const db = this.db;
                await new Promise((resolve, reject) => {
                    db.all('SELECT * FROM foo', (err, rows) => {
                        if (err) reject(err);
                        else resolve();
                    });
                });
            },

            async afterEach() {
                // Teardown: Close database - NOT measured
                await promisifyClose(this.db);
            },
        },

        'db.all with statement reset': {
            async beforeEach() {
                // Setup: Create and populate database - NOT measured
                this.db = new sqlite3.Database(':memory:');
                await promisifyExec(this.db, setupSQL);
            },

            async fn() {
                // Benchmark: Retrieve all results with statement reset - MEASURED
                const db = this.db;
                await new Promise((resolve, reject) => {
                    const stmt = db.prepare('SELECT * FROM foo');
                    stmt.all((err, rows) => {
                        if (err) {
                            stmt.finalize();
                            reject(err);
                            return;
                        }
                        stmt.reset();
                        stmt.all((err2, rows2) => {
                            if (err2) {
                                stmt.finalize();
                                reject(err2);
                                return;
                            }
                            stmt.finalize();
                            resolve();
                        });
                    });
                });
            },

            async afterEach() {
                // Teardown: Close database - NOT measured
                await promisifyClose(this.db);
            },
        },
    },
};
