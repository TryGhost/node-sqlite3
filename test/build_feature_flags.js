var sqlite3 = require('..');
var assert = require('assert');
var fs = require('fs');

describe('build feature flags', function() {
  var db;

  beforeEach(function(done) {
    db = new sqlite3.Database(':memory:');
    db.run("CREATE TABLE waffles (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, number INTEGER)", done);
  });

  it('allows UPDATE ... limit', function(done) {
    var sql = "UPDATE waffles SET number = number - ? WHERE number >= ? ORDER BY id ASC LIMIT 1";

    db.run(sql, [1,1], function(err) {
      if (err) throw err;

      done();
    });
  });
});

