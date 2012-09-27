/*jshint strict:true node:true es5:true onevar:true laxcomma:true laxbreak:true eqeqeq:true immed:true latedef:true*/
(function () {
  "use strict";

  var sqlite = require('sqlite3')
    , assert = require('assert')
    , db
    ;

  function readyForQuery() {
    db.exec('CREATE VIRTUAL TABLE t1 USING fts4(content="", a, b, c);', function (err) {
      assert.ok(!err);
    });
  }

  db = new sqlite.Database(":memory:", readyForQuery);
}());
