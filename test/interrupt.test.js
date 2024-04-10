const sqlite3 = require('..');
const assert = require('assert');

describe('interrupt', function() {
    it('should interrupt queries', async function() {
        let interrupted = false;
        let saved = null;

        const db = await sqlite3.Database.create(':memory:');
        await db.serialize(async () => {
    
            let setup = 'create table t (n int);';
            for (let i = 0; i < 8; i += 1) {
                setup += 'insert into t values (' + i + ');';
            }
    
            await db.exec(setup);
    
            const query = 'select last.n ' +
                'from t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t as last';
    
            try {
                for await (const _row of await db.each(query)) {
                    if (!interrupted) {
                        interrupted = true;
                        db.interrupt();
                    }
                }
            } catch (err) {
                saved = err;
            } finally {
                await db.close();
            }
    
            if (!saved) throw new Error('Completed query without error, but expected error');
            assert.equal(saved.message, 'SQLITE_INTERRUPT: interrupted');
            assert.equal(saved.errno, sqlite3.INTERRUPT);
            assert.equal(saved.code, 'SQLITE_INTERRUPT');
        });
        
    });

    it('should throw if interrupt is called before open', async function() {
        // @ts-expect-error Calling private constructor for testing
        const db = new sqlite3.Database(':memory:');

        assert.throws(function() {
            db.interrupt();
        }, (/Database is not open/));
    });

    it('should throw if interrupt is called after close', async function() {
        const db = await sqlite3.Database.create(':memory:');

        await db.close();

        assert.throws(function() {
            db.interrupt();
        }, (/Database is not open/));
    });

    it('should throw if interrupt is called during close', async function() {
        const db = await sqlite3.Database.create(':memory:');
        db.close();

        assert.throws(function() {
            db.interrupt();
        }, (/Database is closing/));
    });
});
