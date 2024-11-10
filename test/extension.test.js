const { join } = require('node:path');
const sqlite3 = require('..');
const { cwd } = require('node:process');
const { exec: execCb } = require('node:child_process');
const {promisify} = require('node:util');

const exec = promisify(execCb);

const uuidExtension = join(cwd(), 'test', 'support', 'uuid.c');


describe('loadExtension', function() {
    before(async function() {
        await exec(`gcc -g -fPIC -shared ${uuidExtension} -o ${uuidExtension}.so`);
    });

    it('can load the uuid extension', async function() {
        const db = await sqlite3.Database.create(':memory:');
        await db.loadExtension(uuidExtension);
    });
});
