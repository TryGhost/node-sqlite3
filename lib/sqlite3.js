const path = require('path');
const sqlite3 = require('./sqlite3-binding.js');
const EventEmitter = require('events').EventEmitter;
module.exports = exports = sqlite3;

function normalizeMethod (fn) {
    return async function (sql) {
        const args = Array.prototype.slice.call(arguments, 1);
        const statement = await Statement.create(this, sql);
        return fn.call(this, statement, args);
    };
}

function inherits(target, source) {
    for (const k in source.prototype)
        target.prototype[k] = source.prototype[k];
}

sqlite3.cached = {
    Database: {
        create: async function(file, a, b) {
            if (file === '' || file === ':memory:') {
                // Don't cache special databases.
                return Database.create(file, a, b);
            }

            let db;
            file = path.resolve(file);

            if (!sqlite3.cached.objects[file]) {
                db = sqlite3.cached.objects[file] = await Database.create(file, a, b);
            }
            else {
                // Make sure the callback is called.
                db = sqlite3.cached.objects[file];
            }

            return db;
        },
    },
    objects: {}
};


const Database = sqlite3.Database;
const Statement = sqlite3.Statement;
const Backup = sqlite3.Backup;

inherits(Database, EventEmitter);
inherits(Statement, EventEmitter);
inherits(Backup, EventEmitter);

Database.create = async function(filename, mode) {
    const database = new Database(filename, mode);
    return database.connect();
};

// Database#prepare(sql, [bind1, bind2, ...])
Database.prototype.prepare = normalizeMethod(function(statement, params) {
    return params.length
        ? statement.bind(...params)
        : statement;
});

// Database#run(sql, [bind1, bind2, ...])
Database.prototype.run = normalizeMethod(function(statement, params) {
    return statement.run(...params)
        .then((result) => statement.finalize()
            .then(() => result));
});

// Database#get(sql, [bind1, bind2, ...])
Database.prototype.get = normalizeMethod(function(statement, params) {
    return statement.get(...params)
        .then((result) => statement.finalize()
            .then(() => result));
});

// Database#all(sql, [bind1, bind2, ...])
Database.prototype.all = normalizeMethod(function(statement, params) {
    return statement.all(...params)
        .then((result) => statement.finalize()
            .then(() => result));
});

// Database#each(sql, [bind1, bind2, ...], [complete])
Database.prototype.each = normalizeMethod(async function*(statement, params) {
    try {
        for await (const row of statement.each(...params)) {
            yield row;
        }
    } finally {
        await statement.finalize();
    } 
});

Database.prototype.map = normalizeMethod(function(statement, params) {
    return statement.map(...params)
        .then((result) => statement.finalize()
            .then(() => result));
});

Backup.create = async function(...args) {
    const backup = new Backup(...args);
    return backup.initialize();
};

// Database#backup(filename)
// Database#backup(filename, destName, sourceName, filenameIsDest)
Database.prototype.backup = async function(...args) {
    let backup;
    if (args.length <= 2) {
        // By default, we write the main database out to the main database of the named file.
        // This is the most likely use of the backup api.
        backup = await Backup.create(this, arguments[0], 'main', 'main', true, arguments[1]);
    } else {
        // Otherwise, give the user full control over the sqlite3_backup_init arguments.
        backup = await Backup.create(this, arguments[0], arguments[1], arguments[2], arguments[3], arguments[4]);
    }
    // Per the sqlite docs, exclude the following errors as non-fatal by default.
    backup.retryErrors = [sqlite3.BUSY, sqlite3.LOCKED];
    return backup;
};

Statement.create = async function(database, sql) {
    const statement = new Statement(database, sql);
    return statement.prepare();
};

Statement.prototype.map = async function(...args) {
    const rows = await this.all(...args);
    const result = {};
    if (rows.length) {
        const keys = Object.keys(rows[0]);
        const key = keys[0];
        if (keys.length > 2) {
            // Value is an object
            for (let i = 0; i < rows.length; i++) {
                result[rows[i][key]] = rows[i];
            }
        } else {
            const value = keys[1];
            // Value is a plain value
            for (let i = 0; i < rows.length; i++) {
                result[rows[i][key]] = rows[i][value];
            }
        }
    }
    
    return result;
};

let isVerbose = false;

const supportedEvents = [ 'trace', 'profile', 'change' ];

Database.prototype.addListener = Database.prototype.on = function(type) {
    const val = EventEmitter.prototype.addListener.apply(this, arguments);
    if (supportedEvents.indexOf(type) >= 0) {
        this.configure(type, true);
    }
    return val;
};

Database.prototype.removeListener = function(type) {
    const val = EventEmitter.prototype.removeListener.apply(this, arguments);
    if (supportedEvents.indexOf(type) >= 0 && !this._events[type]) {
        this.configure(type, false);
    }
    return val;
};

Database.prototype.removeAllListeners = function(type) {
    const val = EventEmitter.prototype.removeAllListeners.apply(this, arguments);
    if (supportedEvents.indexOf(type) >= 0) {
        this.configure(type, false);
    }
    return val;
};

// Save the stack trace over EIO callbacks.
sqlite3.verbose = function() {
    if (!isVerbose) {
        const trace = require('./trace');
        [
            'prepare',
            'get',
            'run',
            'all',
            'each',
            'map',
            'close',
            'exec'
        ].forEach(function (name) {
            trace.extendTrace(Database.prototype, name);
        });
        [
            'bind',
            'get',
            'run',
            'all',
            'each',
            'map',
            'reset',
            'finalize',
        ].forEach(function (name) {
            trace.extendTrace(Statement.prototype, name);
        });
        isVerbose = true;
    }

    return sqlite3;
};
