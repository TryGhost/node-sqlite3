var sqlite3 = require('..');
var assert = require('assert');
var helper = require('./support/helper');
var util = require('util');

describe('cache', function() {
    const filename = 'test/tmp/test_cache.db';
    const dbs = [];

    const open = async (filename) => await new Promise((resolve, reject) => {
        new sqlite3.cached.Database(filename, function (err) {
            if (err != null) return reject(err);
            resolve(this);
        });
    });
    const close = async (db) => await util.promisify(db.close.bind(db))();

    beforeEach(async function () {
        dbs.length = 0;
        helper.ensureExists('test/tmp');
        helper.deleteFile(filename);
    });

    afterEach(async function () {
        await Promise.all(dbs.map(async (db) => await close(db)));
        dbs.length = 0;
        helper.deleteFile(filename);
    });

    it('should cache Database objects while opening', function(done) {
        var opened1 = false, opened2 = false;
        dbs.push(new sqlite3.cached.Database(filename, function(err) {
            if (err) throw err;
            opened1 = true;
            if (opened1 && opened2) done();
        }));
        dbs.push(new sqlite3.cached.Database(filename, function(err) {
            if (err) throw err;
            opened2 = true;
            if (opened1 && opened2) done();
        }));
        assert.equal(dbs[0], dbs[1]);
    });

    it('should cache Database objects after they are open', function(done) {
        dbs.push(new sqlite3.cached.Database(filename, function(err) {
            if (err) throw err;
            process.nextTick(function() {
                dbs.push(new sqlite3.cached.Database(filename, function(err) {
                    if (err) throw err;
                    done();
                }));
                assert.equal(dbs[0], dbs[1]);
            });
        }));
    });

    it('cached.Database() callback is called asynchronously', async function () {
        await Promise.all([0, 1].map(() => new Promise((resolve, reject) => {
            let callbackCalled = false;
            dbs.push(new sqlite3.cached.Database(filename, (err) => {
                callbackCalled = true;
                if (err != null) return reject(err);
                resolve();
            }));
            assert(!callbackCalled);
        })));
    });

    it('cached.Database() callback is called with db as context', async function () {
        await Promise.all([0, 1].map((i) => new Promise((resolve, reject) => {
            dbs.push(new sqlite3.cached.Database(filename, function (err) {
                if (err != null) return reject(err);
                if (this !== dbs[i]) return reject(new Error('this !== dbs[i]'));
                resolve();
            }));
        })));
    });

    it('db.close() callback is called asynchronously', async function () {
        dbs.push(await open(filename));
        dbs.push(await open(filename));
        while (dbs.length > 0) {
            await new Promise((resolve, reject) => {
                let callbackCalled = false;
                dbs.pop().close((err) => {
                    callbackCalled = true;
                    if (err != null) return reject(err);
                    resolve();
                });
                assert(!callbackCalled);
            });
        }
    });

    it('db.close() callback is called with db as context', async function () {
        dbs.push(await open(filename));
        dbs.push(await open(filename));
        while (dbs.length > 0) {
            await new Promise((resolve, reject) => {
                const db = dbs.pop();
                db.close(function (err) {
                    if (err) return reject(err);
                    if (this !== db) return reject(new Error('this !== db'));
                    resolve();
                });
            });
        }
    });

    it('db.close() does not close other copies', async function () {
        dbs.push(await open(filename));
        dbs.push(await open(filename));
        await close(dbs.pop());
        assert(dbs[0].open);
    });

    it('db.close() closes the underlying Database after closing the last copy', async function () {
        dbs.push(await open(filename));
        dbs.push(await open(filename));
        const db = dbs[0];
        await close(dbs.pop());
        await close(dbs.pop());
        assert(!db.open);
    });

    it('cached.Database() returns an open Database after closing', async function () {
        dbs.push(await open(filename));
        await close(dbs.pop());
        dbs.push(await open(filename));
        assert(dbs[0].open);
    });
});
