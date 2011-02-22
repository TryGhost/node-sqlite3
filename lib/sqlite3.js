var sqlite3 = module.exports = exports = require('./sqlite3_bindings');
var sys = require("sys");


var Database = sqlite3.Database;
var Statement = sqlite3.Statement;

// Database#prepare(sql, [bind1, bind2, ...], [callback])
Database.prototype.prepare = function(sql) {
    var callback, params = Array.prototype.slice.call(arguments, 1);

    if (params.length && typeof params[params.length - 1] !== 'function') {
        params.push(undefined);
    }

    if (params.length > 1) {
        var statement = new Statement(this, sql);
        return statement.bind.apply(statement, params);
    } else {
        return new Statement(this, sql, params.pop());
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

Database.prototype.execute = function() {
    console.warn('Database#execute() is deprecated. Use Database#all() instead.');
    return this.all.apply(this, arguments);
};
