module.exports = require('./sqlite3');

var promiseVersion = require('./promise-sqlite3');
module.exports.PromiseStatement = promiseVersion.PromiseStatement;
module.exports.PromiseDatabase = promiseVersion.PromiseDatabase;