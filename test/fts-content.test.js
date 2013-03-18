var sqlite = require('sqlite3')
var assert = require('assert')

exports['test fts content'] = function(beforeExit) {
    var db = new sqlite.Database(":memory:");
    db.exec('CREATE VIRTUAL TABLE t1 USING fts4(content="", a, b, c);', function (err) {
      assert.ok(!err);
    });
}
