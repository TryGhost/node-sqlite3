var sqlite3 = require('sqlite3');
var assert = require('assert');

if (process.setMaxListeners) process.setMaxListeners(0);

a = exports['test running the sync routines'] = function(beforeExit) {

    var count = 10;
    var inserted = 0;
    var retrieved = 0;

    new sqlite3.Database(':memory:', function(err) {
      if (err) throw err;

      var db = this;
      db.execSync("CREATE TABLE foo (id int)");

      db.execSync("INSERT INTO foo VALUES(1)");
      inserted++;

      db.prepare("INSERT INTO foo VALUES(?)", function(err, stmt) {
        if (err) throw err;
        for (var i = 5; i < count; i++) {
            this.runSync(i);
            inserted++;
        }

        db.prepare("SELECT id FROM foo ORDER BY id", function(err, stmt) {
          if (err) throw err;
          var collected = this.allSync();

          retrieved += collected.length;
          assert.deepEqual(collected, [ {id: 1 }, { id: 5 }, { id: 6 }, { id: 7 }, { id: 8 }, { id: 9 } ]);

          new sqlite3.Database(':memory:', function(err) {
              this.copy(db);

              this.prepare("SELECT id FROM foo ORDER BY id", function(err, stmt) {
                 if (err) throw err;
                 var collected = this.allSync();
                 assert.deepEqual(collected, [ {id: 1 }, { id: 5 }, { id: 6 }, { id: 7 }, { id: 8 }, { id: 9 } ]);
              }); 
          });
        });
      })
    });

    beforeExit(function() {
      assert.equal(inserted, 6);
      assert.equal(retrieved, 6);
    });
};

a(function(){})
