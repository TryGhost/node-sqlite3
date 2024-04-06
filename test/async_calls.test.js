"use strict";

var sqlite3 = require('..');
const assert = require("assert");
const { createHook, executionAsyncId } = require("async_hooks");


// TODO: What.. is this testing??
describe.skip('async_hooks', function() {
    /** @type {sqlite3.Database} */
    let db;
    let dbId;
    let asyncHook;

    beforeEach(async function() {
        db = await sqlite3.Database.create(':memory:');

        asyncHook = createHook({
            init(asyncId, type) {
                if (dbId == null && type.startsWith("sqlite3.")) {
                    dbId = asyncId;
                }
            }
        }).enable();
    });

    it('should support performance measuring with async hooks', async function() {
        await db.run("DROP TABLE user");
        const cbId = executionAsyncId();
        assert.strictEqual(cbId, dbId);
    });

    afterEach(function() {
        if (asyncHook != null) {
            asyncHook.disable();
        }
        dbId = null;
        if (db != null) {
            db.close();
        }
    });
});
