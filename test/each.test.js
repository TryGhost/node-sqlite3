var sqlite3 = require('sqlite3');
var assert = require('assert');
var util = require('util');

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