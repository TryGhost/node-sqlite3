var Benchmark = require('benchmark');
var sqlite3 = require('../lib/sqlite3');


var suite = new Benchmark.Suite;

var iterations = 10000;

// add tests
suite
    .add('insert without transaction', function() {
        var bench = this;
        var db = new sqlite3.Database('');
        
        db.serialize(function() {
            db.run("CREATE TABLE foo (id INT, txt TEXT)");
            var stmt = db.prepare("INSERT INTO foo VALUES (?, ?)");
            for (var i = 0; i < iterations; i++) {
                stmt.run(i, 'Row ' + i);
            }
            stmt.finalize();
        });
        db.close(function() {
            bench.emit('complete');
        });
    }, {
        onComplete: function() {
            console.log('compelte2');
        }
    })
    .add('insert with transaction', function() {
        'Hello World!'.indexOf('o') > -1;
    })



// add listeners
suite
    .on('cycle', function(bench) {
        console.log(String(bench));
    })
    .on('complete', function() {
        console.log('Fastest is ' + this.filter('fastest').pluck('name'));
    })
    .run(true);