var sqlite = require('sqlite3_bindings'),
    Step = require('step'),
    fs = require('fs'),
    assert = require('assert');

var db = new sqlite.Database();

// lots of elmo
var elmo = fs.readFile(__dirname + '/elmo.png', 'binary');

exports['Blob overflow test'] = function() {
    Step(
        function() {
            db.open(__dirname + '/blobtest.sqlite', this);
        },
        function() {
            db.prepare(
            'CREATE TABLE IF NOT EXISTS elmos ('
            + 'image BLOB);',
            function(err, statement) {
                assert.isUndefined(err);
                statement.step(this);
            }.bind(this));
        },
        function(err) {
            db.prepare(
            'INSERT INTO elmos (image) '
            + 'VALUES (?)',
            function(err, statement) {
                assert.isUndefined(err);
                statement.bind(0, elmo);
                statement.step(this);
            }.bind(this));
        },
        function() {
            var group = this.group();
            for (var i = 0; i < 1000000; i++) {
                console.log('ELMO');
                db.prepare(
                'INSERT INTO elmos (image) '
                + 'VALUES (?)',
                function(err, statement) {
                    Step(
                        function() {
                            statement.bind(0, elmo, this);
                        },
                        function() {
                            // DO IT
                            statement.step(group());
                        }
                    );
                }.bind(this));
            }
        }
    );
}
