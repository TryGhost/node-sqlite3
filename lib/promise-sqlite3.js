'use strict';

var sqlite3 = require('./sqlite3');

// Utility functions for Statement
function promiseFor(fn, args) {
    var object = this;
    return new Promise(function (resolve, reject) {
        args.push(function (err) {
            if (err) {
                return reject(err);
            } else {
                var results = Array.prototype.slice.call(arguments, 1);
                return resolve.apply(undefined, results);
            }
        });
        fn.apply(object, args);
    });
}

// Statement
function PromiseStatement(database, sql, callback) {
    this.statement = new sqlite3.Statement(database, sql, callback);
}
for (var sk in sqlite3.Statement.prototype) {
    PromiseStatement.prototype[sk] = function () {
        return sqlite3.Statement.prototype[sk].apply(this.statement, arguments);
    };
}

PromiseStatement.prototype.bind = function () {
    return promiseFor.call(this.statement, sqlite3.Statement.prototype.bind, arguments);
};

PromiseStatement.prototype.get = function () {
    return promiseFor.call(this.statement, sqlite3.Statement.prototype.get, arguments);
};

PromiseStatement.prototype.run = function () {
    return promiseFor.call(this.statement, sqlite3.Statement.prototype.run, arguments);
};

PromiseStatement.prototype.all = function () {
    return promiseFor.call(this.statement, sqlite3.Statement.prototype.all, arguments);
};

PromiseStatement.prototype.each = function () {
    return promiseFor.call(this.statement, sqlite3.Statement.prototype.each, arguments);
};

PromiseStatement.prototype.map = function () {
    return promiseFor.call(this.statement, sqlite3.Statement.prototype.map, arguments);
};

PromiseStatement.prototype.reset = function () {
    return promiseFor.call(this.statement, sqlite3.Statement.prototype.reset, []);
};

PromiseStatement.prototype.finalize = function () {
    return promiseFor.call(this.statement, sqlite3.Statement.prototype.finalize, []);
};

// Utility functions for Database
function checkOpen() {
    if (!this.database) {
        throw {
            "errno": 21,
            "code": "SQLITE_MISUSE",
            "value": "SQLITE_MISUSE: Database was not open"
        };
    }
}

function normalizeMethod (fn) {
    return function (sql) {
        checkOpen.call(this);
        var args = Array.prototype.slice.call(arguments, 1);
        var that = this;
        return new Promise(function (resolve, reject) {
            var statement = new PromiseStatement(that.database, sql, function (err) {
                if (err) {
                    return reject(err);
                }
            });
            fn.call(that, statement, args).then(function () {
                return resolve(arguments);
            }).catch(function (err) {
                return reject(err);
            });
        });
    };
}

function normalizeMethodAndFinalize(fn) {
    return normalizeMethod.call(this, function (statement, params) {
        var that = this;
        return new Promise(function (resolve, reject) {
            var result;
            fn.call(that, statement, params).then(function () {
                result = arguments;
                return statement.finalize();
            }).then(function () {
                return resolve(result);
            }).catch(function (err) {
                return reject(err);
            });
        });
    });
}

// Database
function PromiseDatabase(filename) {
    this.database = null;
    this.storedOptions = {};
    this.filename = filename;
}
for (var dk in sqlite3.Database.prototype) {
    PromiseDatabase.prototype[dk] = function () {
        checkOpen.call(this);
        return sqlite3.Statement.prototype[dk].apply(this.database, arguments);
    };
}

PromiseDatabase.prototype.configure = function (option, value) {
    if (this.database) {
        this.database.configure(option, value);
    } else {
        this.storedOptions[option] = value;
    }
};

PromiseDatabase.prototype.open = function (mode) {
    var that = this;
    if (mode === undefined) {
        mode = sqlite3.OPEN_READWRITE | sqlite3.OPEN_CREATE;
    }
    return new Promise(function (resolve, reject) {
        if (that.database) {
            throw {
                "errno": 21,
                "code": "SQLITE_MISUSE",
                "value": "SQLITE_MISUSE: Database is already open"
            };
        }
        that.database = new sqlite3.Database(that.filename, mode, function (err) {
            if (err) {
                reject(err);
            } else {
                resolve();
            }
        });
        for (var opt in that.storedOptions) {
            that.database.configure(opt, that.storedOptions[opt]);
        }
    });
};

PromiseDatabase.prototype.prepare = normalizeMethod(function (statement, params) {
    if (params.length) {
        return statement.bind.apply(statement, params);
    } else {
        return new Promise(function (resolve) {
            return resolve(statement);
        });
    }
});

PromiseDatabase.prototype.run = normalizeMethodAndFinalize(function (statement, params) {
    return statement.run.apply(statement, params);
});

PromiseDatabase.prototype.get = normalizeMethodAndFinalize(function(statement, params) {
    return statement.get.apply(statement, params);
});

PromiseDatabase.prototype.all = normalizeMethodAndFinalize(function(statement, params) {
    return statement.all.apply(statement, params);
});

PromiseDatabase.prototype.each = normalizeMethodAndFinalize(function(statement, params) {
    return statement.each.apply(statement, params);
});

PromiseDatabase.prototype.map = normalizeMethodAndFinalize(function(statement, params) {
    return statement.map.apply(statement, params);
});

// Exports
module.exports.PromiseStatement = PromiseStatement;
module.exports.PromiseDatabase = PromiseDatabase;