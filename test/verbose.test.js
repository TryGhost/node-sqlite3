const sqlite3 = require('..');
const assert = require('assert');

const invalidSql = 'update non_existent_table set id=1';

const originalMethods = {
    Database: {},
    Statement: {},
};

function backupOriginalMethods() {
    for (const obj in originalMethods) {
        for (const attr in sqlite3[obj].prototype) {
            originalMethods[obj][attr] = sqlite3[obj].prototype[attr];
        }
    }
}

function resetVerbose() {
    for (const obj in originalMethods) {
        for (const attr in originalMethods[obj]) {
            sqlite3[obj].prototype[attr] = originalMethods[obj][attr];
        }
    }
}

describe('verbose', function() {
    it('Shoud add trace info to error when verbose is called', async function() {
        const db = await sqlite3.Database.create(':memory:');
        backupOriginalMethods();
        sqlite3.verbose();

        await assert.rejects(db.run(invalidSql), function(err) {
            assert.ok(err instanceof Error);

            assert.ok(
                err.stack.indexOf(`Database#run('${invalidSql}'`) > -1,
                `Stack shoud contain trace info, stack = ${err.stack}`
            );

            resetVerbose();
            return true;
        });
    });

    it('Shoud not add trace info to error when verbose is not called', async function() {
        const db = await sqlite3.Database.create(':memory:');

        await assert.rejects(db.run(invalidSql), function(err) {
            assert.ok(err instanceof Error);

            assert.ok(
                err.stack.indexOf(invalidSql) === -1,
                `Stack shoud not contain trace info, stack = ${err.stack}`
            );

            return true;
        });
    });
});
