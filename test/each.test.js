var sqlite3 = require('sqlite3');
var assert = require('assert');
var util = require('util');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test Statement#each'] = function(beforeExit) {
    var db = new sqlite3.Database('test/support/big.db', sqlite3.OPEN_READONLY);

    var total = 100000;
    var retrieved = 0;

    db.each('SELECT id, txt FROM foo LIMIT 0, ?', total, function(err, row) {
        if (err) throw err;
        retrieved++;
    });

    beforeExit(function() {
        assert.equal(retrieved, total, "Only retrieved " + retrieved + " out of " + total + " rows.");
    });
};
// 
// exports['test Statement#each with complete callback'] = function(beforeExit) {
//     var db = new sqlite3.Database('test/support/big.db', sqlite3.OPEN_READONLY);
// 
//     var total = 10000;
//     var retrieved = 0;
//     var completed = false;
// 
//     db.each('SELECT id, txt FROM foo LIMIT 0, ?', total, function(err, row) {
//         if (err) throw err;
//         retrieved++;
//     }, function(err, num) {
//         assert.equal(retrieved, num);
//         completed = true;
//     });
// 
//     beforeExit(function() {
//         assert.ok(completed);
//         assert.equal(retrieved, total, "Only retrieved " + retrieved + " out of " + total + " rows.");
//     });
// };
