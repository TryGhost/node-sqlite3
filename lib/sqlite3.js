var sqlite3 = module.exports = exports = require('./sqlite3_bindings');

var Database = sqlite3.Database;
var Statement = sqlite3.Statement;

// Database#prepare(sql, [bind1, bind2, ...], [callback])
Database.prototype.prepare = function(sql) {
    var callback, params = Array.prototype.slice.call(arguments, 1);

    if (!params.length || (params.length === 1 && typeof params[0] === 'function')) {
        return new Statement(this, sql, params[0]);
    }
    else {
        var statement = new Statement(this, sql, function(err) {
            if (err && typeof params[params.length - 1] === 'function') {
                params[params.length - 1](err);
            }
        });
        return statement.bind.apply(statement, params);
    }
};

// Database#run(sql, [bind1, bind2, ...], [callback])
Database.prototype.run = function(sql) {
    var statement = new Statement(this, sql);
    statement.run.apply(statement, Array.prototype.slice.call(arguments, 1)).finalize();
    return this;
}

// Database#get(sql, [bind1, bind2, ...], [callback])
Database.prototype.get = function(sql) {
    var statement = new Statement(this, sql);
    statement.get.apply(statement, Array.prototype.slice.call(arguments, 1)).finalize();
    return this;
}

// Database#all(sql, [bind1, bind2, ...], [callback])
Database.prototype.all = function(sql) {
    var statement = new Statement(this, sql);
    statement.all.apply(statement, Array.prototype.slice.call(arguments, 1)).finalize();
    return this;
}

// Database#each(sql, [bind1, bind2, ...], [callback])
Database.prototype.each = function(sql) {
    var statement = new Statement(this, sql);
    statement.each.apply(statement, Array.prototype.slice.call(arguments, 1)).finalize();
    return this;
}

// Save the stack trace over EIO callbacks.
sqlite3.verbose = function() {
    var trace = require('./trace');
    trace.extendTrace(Database.prototype, 'prepare');
    trace.extendTrace(Database.prototype, 'get');
    trace.extendTrace(Database.prototype, 'run');
    trace.extendTrace(Database.prototype, 'all');
    trace.extendTrace(Database.prototype, 'each');
    trace.extendTrace(Database.prototype, 'exec');
    trace.extendTrace(Database.prototype, 'close');
    trace.extendTrace(Statement.prototype, 'bind');
    trace.extendTrace(Statement.prototype, 'get');
    trace.extendTrace(Statement.prototype, 'run');
    trace.extendTrace(Statement.prototype, 'all');
    trace.extendTrace(Statement.prototype, 'each');
    trace.extendTrace(Statement.prototype, 'reset');
    trace.extendTrace(Statement.prototype, 'finalize');
    return this;
};
