var sqlite = require('sqlite3'),
    Step = require('step'),
    fs = require('fs'),
    assert = require('assert')
    Buffer = require('buffer').Buffer;

// lots of elmo
var elmo = fs.readFileSync(__dirname + '/support/elmo.png', 'binary');
var elmo_str = elmo.toString('binary');

exports['Blob overflow test'] = function(beforeExit) {
    var db = new sqlite.Database();
    var total = 100;
    var inserted = 0;
    var retrieved = 0;

    Step(
        function() {
            db.open('', this);
        },
        function() {
            var next = this;
            db.prepare('CREATE TABLE elmos (image BLOB);', function(err, statement) {
                assert.isUndefined(err);
                statement.step(next);
            });
        },
        function() {
            var group = this.group();
            for (var i = 0; i < total; i++) {
                var next = group();
                db.prepare('INSERT INTO elmos (image) VALUES (?)', function(err, statement) {
                    assert.isUndefined(err);
                    statement.bind(1, elmo, function() {
                        statement.step(function(err) {
                            assert.isUndefined(err);
                            inserted++;
                            next();
                        });
                    });
                });
            }
        },
        function() {
            var next = this;
            db.execute('SELECT COUNT(*) as amount FROM elmos', function(err, rows) {
                assert.isUndefined(err);
                assert.eql(rows[0].amount, total);
                next();
            });
        },
        function() {
            var next = this;
            db.prepare('SELECT image FROM elmos;', function(err, statement) {
                assert.isUndefined(err);
                fetch();

                function fetch() {
                    statement.step(function(err, row) {
                        assert.isUndefined(err);
                        if (row) {
                            // Not using assert.equal here because it's image data
                            // and we don't want that in the command line.
                            assert.ok(elmo_str === row.image);
                            retrieved++;
                            fetch();
                        }
                        else {
                            next();
                        }
                    });
                }
            });
        }
    );

    beforeExit(function() {
        assert.eql(inserted, total);
        assert.eql(retrieved, total);
    })
}
