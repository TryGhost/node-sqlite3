var sqlite3 = require('sqlite3');
var assert = require('assert');

if (process.setMaxListeners) process.setMaxListeners(0);

function randomString() {
    var str = '';
    for (var i = Math.random() * 300; i > 0; i--) {
        str += String.fromCharCode(Math.floor(Math.random() * 65536));
    }
    return str;
};

exports['test unicode characters'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    // Generate random data.
    var data = [];
    var length = Math.floor(Math.random() * 1000) + 200;
    for (var i = 0; i < length; i++) {
        data.push(randomString());
    }

    var inserted = 0;
    var retrieved = 0;

    db.serialize(function() {
       db.run("CREATE TABLE foo (id int, txt text)");

       var stmt = db.prepare("INSERT INTO foo VALUES(?, ?)");
       for (var i = 0; i < data.length; i++) {
           stmt.run(i, data[i], function(err) {
               if (err) throw err;
               inserted++;
           });
       }
       stmt.finalize();

       db.all("SELECT txt FROM foo ORDER BY id", function(err, rows) {
           if (err) throw err;

           for (var i = 0; i < rows.length; i++) {
               assert.equal(rows[i].txt, data[i]);
               retrieved++;
           }
       });
    });

    beforeExit(function() {
        assert.equal(inserted, length);
        assert.equal(retrieved, length);
    });
};
