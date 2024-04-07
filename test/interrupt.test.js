var sqlite3 = require('..');
var assert = require('assert');

describe('interrupt', function() {
    // TODO: each
    it.skip('should interrupt queries', async function() {
        let interrupted = false;
        let saved = null;

        const db = await sqlite3.Database.create(':memory:');
        await db.serialize();

        var setup = 'create table t (n int);';
        for (var i = 0; i < 8; i += 1) {
            setup += 'insert into t values (' + i + ');';
        }

        await db.exec(setup);

        const query = 'select last.n ' +
            'from t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t as last';

        db.each(query, function(err) {
            if (err) {
                saved = err;
            } else if (!interrupted) {
                interrupted = true;
                db.interrupt();
            }
        });

        await db.close();

        if (saved) {
            assert.equal(saved.message, 'SQLITE_INTERRUPT: interrupted');
            assert.equal(saved.errno, sqlite3.INTERRUPT);
            assert.equal(saved.code, 'SQLITE_INTERRUPT');
        }
        new Error('Completed query without error, but expected error');
    });

    it('should throw if interrupt is called before open', function(done) {
        var db = new sqlite3.Database(':memory:');

        assert.throws(function() {
            db.interrupt();
        }, (/Database is not open/));

        db.close();
        done();
    });

    it('should throw if interrupt is called after close', function(done) {
        var db = new sqlite3.Database(':memory:');

        db.close(function() {
            assert.throws(function() {
                db.interrupt();
            }, (/Database is not open/));

            done();
        });
    });

    it('should throw if interrupt is called during close', function(done) {
        var db = new sqlite3.Database(':memory:', function() {
            db.close();
            assert.throws(function() {
                db.interrupt();
            }, (/Database is closing/));
            done();
        });
    });
});
