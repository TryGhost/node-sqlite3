var binary = require('node-pre-gyp');
var path = require('path');
var binding_path = binary.find(path.resolve(path.join(__dirname,'../package.json')));
var binding = require(binding_path);
var sqlite3 = module.exports = exports = binding;
var EventEmitter = require('events').EventEmitter;

function normalizeMethod (fn) {
    return function (sql) {
        var errBack;
        var args = Array.prototype.slice.call(arguments, 1);
        if (typeof args[args.length - 1] === 'function') {
            var callback = args[args.length - 1];
            errBack = function(err) {
                if (err) {
                    callback(err);
                }
            };
        }
        var statement = new Statement(this, sql, errBack);
        return fn.call(this, statement, args);
    };
}

function inherits(target, source) {
    for (var k in source.prototype)
        target.prototype[k] = source.prototype[k];
}

sqlite3.cached = {
    Database: function(file, a, b) {
        if (file === '' || file === ':memory:') {
            // Don't cache special databases.
            return new Database(file, a, b);
        }

        var db;
        file = path.resolve(file);
        function cb() { callback.call(db, null); }

        if (!sqlite3.cached.objects[file]) {
            db = sqlite3.cached.objects[file] = new Database(file, a, b);
        }
        else {
            // Make sure the callback is called.
            db = sqlite3.cached.objects[file];
            var callback = (typeof a === 'number') ? b : a;
            if (typeof callback === 'function') {
                if (db.open) process.nextTick(cb);
                else db.once('open', cb);
            }
        }

        return db;
    },
    objects: {}
};


var Database = sqlite3.Database;
var Statement = sqlite3.Statement;

inherits(Database, EventEmitter);
inherits(Statement, EventEmitter);

// Database#prepare(sql, [bind1, bind2, ...], [callback])
Database.prototype.prepare = normalizeMethod(function(statement, params) {
    return params.length
        ? statement.bind.apply(statement, params)
        : statement;
});

// Database#run(sql, [bind1, bind2, ...], [callback])
Database.prototype.run = normalizeMethod(function(statement, params) {
    statement.run.apply(statement, params).finalize();
    return this;
});

// Database#get(sql, [bind1, bind2, ...], [callback])
Database.prototype.get = normalizeMethod(function(statement, params) {
    statement.get.apply(statement, params).finalize();
    return this;
});

// Database#all(sql, [bind1, bind2, ...], [callback])
Database.prototype.all = normalizeMethod(function(statement, params) {
    statement.all.apply(statement, params).finalize();
    return this;
});

// Database#each(sql, [bind1, bind2, ...], [callback], [complete])
Database.prototype.each = normalizeMethod(function(statement, params) {
    statement.each.apply(statement, params).finalize();
    return this;
});

Database.prototype.map = normalizeMethod(function(statement, params) {
    statement.map.apply(statement, params).finalize();
    return this;
});

Statement.prototype.map = function() {
    var params = Array.prototype.slice.call(arguments);
    var callback = params.pop();
    params.push(function(err, rows) {
        if (err) return callback(err);
        var result = {};
        if (rows.length) {
            var keys = Object.keys(rows[0]), key = keys[0];
            if (keys.length > 2) {
                // Value is an object
                for (var i = 0; i < rows.length; i++) {
                    result[rows[i][key]] = rows[i];
                }
            } else {
                var value = keys[1];
                // Value is a plain value
                for (i = 0; i < rows.length; i++) {
                    result[rows[i][key]] = rows[i][value];
                }
            }
        }
        callback(err, result);
    });
    return this.all.apply(this, params);
};

var isVerbose = false;

var supportedEvents = [ 'trace', 'profile', 'insert', 'update', 'delete' ];

Database.prototype.addListener = Database.prototype.on = function(type) {
    var val = EventEmitter.prototype.addListener.apply(this, arguments);
    if (supportedEvents.indexOf(type) >= 0) {
        this.configure(type, true);
    }
    return val;
};

Database.prototype.removeListener = function(type) {
    var val = EventEmitter.prototype.removeListener.apply(this, arguments);
    if (supportedEvents.indexOf(type) >= 0 && !this._events[type]) {
        this.configure(type, false);
    }
    return val;
};

Database.prototype.removeAllListeners = function(type) {
    var val = EventEmitter.prototype.removeAllListeners.apply(this, arguments);
    if (supportedEvents.indexOf(type) >= 0) {
        this.configure(type, false);
    }
    return val;
};

['all', 'each', 'get'].forEach(function (name) {
    var allCallback = function(rowMode, callback) {
        return function replacement(err, rows, fields) {
            if (err) return callback(err);
            if (!fields) return callback(null, rows);
            var result = [];
            var i, j, row;
            if (rowMode == 'object') {
                for (i = 0; i < rows.length; i++) {
                    row = {};
                    for (j = 0; j < fields.length; j++) {
                        row[fields[j].name] = rows[i][j];
                    }
                    result.push(row);
                }
                callback(null, result);
            }
            else if (rowMode == 'nest') {
                for (i = 0; i < rows.length; i++) {
                    row = {};
                    for (j = 0; j < fields.length; j++) {
                        var table = row[fields[j].table];
                        if (!table) table = row[fields[j].table] = {};
                        table[fields[j].name] = rows[i][j];
                    }
                    result.push(row);
                }
                callback(null, result);
            }
            else {
                callback(null, rows, fields);
            }
        };
    };
    var getCallback = function(rowMode, callback) {
        var cb = allCallback(rowMode, function(err, rows, fields) {
            callback(err, rows ? rows[0] : null, fields);
        });
        return function(err, row, fields) {
            cb(err, [row], fields);
        };
    };
    var wrapCallback = function(name, rowMode, callback) {
        return name == 'all' ? allCallback(rowMode, callback) : getCallback(rowMode, callback);
    };

    var databaseMethodOrig = Database.prototype[name];
    Database.prototype[name] = function () {
        var args = Array.prototype.slice.call(arguments);
        var sql = args.shift();
        var rowMode = 'object';
        if (sql == null) throw new Error('Unexpected sql null');
        if (typeof sql == 'object') {
            if (sql.rowMode) rowMode = sql.rowMode;
            sql = sql.sql;
        }
        var callback = args.pop();
        args.unshift(sql);
        args.push(wrapCallback(name, rowMode, callback));
        return databaseMethodOrig.apply(this, args);
    };

    var statementMethodOrig = Statement.prototype[name];
    Statement.prototype[name] = function () {
        var args = Array.prototype.slice.call(arguments);
        var callback = args.pop();
        args.push(callback.length < 3 ? wrapCallback(name, 'object', callback) : callback);
        return statementMethodOrig.apply(this, args);
    };
});

// Save the stack trace over EIO callbacks.
sqlite3.verbose = function() {
    if (!isVerbose) {
        var trace = require('./trace');
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

    return this;
};
