var path = require('path');
var pkg = require('../package.json');
var module_path = path.join(
     path.join(__dirname,'../' + pkg.binary.module_path),
     pkg.binary.module_name + '.node');
var binding = require(module_path);
var sqlite3 = module.exports = exports = binding;
var util = require('util');
var EventEmitter = require('events').EventEmitter;

function errorCallback(args) {
    if (typeof args[args.length - 1] === 'function') {
        var callback = args[args.length - 1];
        return function(err) { if (err) callback(err); };
    }
}

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}

sqlite3.cached = {
    Database: function(file, a, b) {
        EventEmitter.call(this)

        if (file === '' || file === ':memory:') {
            // Don't cache special databases.
            return new Database(file, a, b);
        }

        if (file[0] !== '/') {
            file = path.join(process.cwd(), file);
        }

        if (!sqlite3.cached.objects[file]) {
            var db =sqlite3.cached.objects[file] = new Database(file, a, b);
        }
        else {
            // Make sure the callback is called.
            var db = sqlite3.cached.objects[file];
            var callback = (typeof a === 'number') ? b : a;
            if (typeof callback === 'function') {
                function cb() { callback.call(db, null); }
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

// Managed counterpart of Database::new
Database.prototype._init = function() {
    EventEmitter.call(this);
};

// Managed counterpart of Statement::new
Statement.prototype._init = function() {
    EventEmitter.call(this);
};

var nativeExec = Database.prototype.exec;
// Database#exec(sql, [callback])
Database.prototype.exec = function(sql) {
    var params = arguments;
    if (process.domain && params.length === 2 && typeof params[1] === 'function') {
        params = Array.prototype.slice.call(params,0);
        // Make sure the callback is called in the current domain.
        params[1] = process.domain.bind(params[1]);
    }
    return nativeExec.apply(this,params);
};

// Database#prepare(sql, [bind1, bind2, ...], [callback])
Database.prototype.prepare = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement;

    if (!params.length || (params.length === 1 && typeof params[0] === 'function')) {
        statement = new Statement(this, sql, params[0]);
    }
    else {
        statement = new Statement(this, sql, errorCallback(params));
        statement = statement.bind.apply(statement, params);
    }

    statement.domain = process.domain;

    return statement
};

// Database#run(sql, [bind1, bind2, ...], [callback])
Database.prototype.run = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));

    statement.domain = process.domain;

    statement.run.apply(statement, params).finalize();
    return this;
};

// Database#get(sql, [bind1, bind2, ...], [callback])
Database.prototype.get = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));

    statement.domain = process.domain;

    statement.get.apply(statement, params).finalize();
    return this;
};

// Database#all(sql, [bind1, bind2, ...], [callback])
Database.prototype.all = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));

    statement.domain = process.domain;

    statement.all.apply(statement, params).finalize();
    return this;
};

// Database#each(sql, [bind1, bind2, ...], [callback], [complete])
Database.prototype.each = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));

    statement.domain = process.domain;

    statement.each.apply(statement, params).finalize();
    return this;
};

Database.prototype.map = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));

    statement.domain = process.domain;

    statement.map.apply(statement, params).finalize();
    return this;
};

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
                for (var i = 0; i < rows.length; i++) {
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

// Save the stack trace over EIO callbacks.
sqlite3.verbose = function() {
    if (!isVerbose) {
        var trace = require('./trace');
        trace.extendTrace(Database.prototype, 'prepare');
        trace.extendTrace(Database.prototype, 'get');
        trace.extendTrace(Database.prototype, 'run');
        trace.extendTrace(Database.prototype, 'all');
        trace.extendTrace(Database.prototype, 'each');
        trace.extendTrace(Database.prototype, 'map');
        trace.extendTrace(Database.prototype, 'exec');
        trace.extendTrace(Database.prototype, 'close');
        trace.extendTrace(Statement.prototype, 'bind');
        trace.extendTrace(Statement.prototype, 'get');
        trace.extendTrace(Statement.prototype, 'run');
        trace.extendTrace(Statement.prototype, 'all');
        trace.extendTrace(Statement.prototype, 'each');
        trace.extendTrace(Statement.prototype, 'map');
        trace.extendTrace(Statement.prototype, 'reset');
        trace.extendTrace(Statement.prototype, 'finalize');
        isVerbose = true;
    }

    return this;
};
