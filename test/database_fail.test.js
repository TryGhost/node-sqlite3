var sqlite3 = require('sqlite3');
var assert = require('assert');

exports['test Database() without new'] = function(beforeExit) {
    assert.throws(function() {
        sqlite3.Database(':memory:');
    }, (/Use the new operator to create new Database objects/));

    assert.throws(function() {
        sqlite3.Statement();
    }, (/Use the new operator to create new Statement objects/));
};

exports['test Database#get prepare fail'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.get('SELECT id, txt FROM foo', function(err, row) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test Database#all prepare fail'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.all('SELECT id, txt FROM foo', function(err, row) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test Database#run prepare fail'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.run('SELECT id, txt FROM foo', function(err, row) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test Database#each prepare fail'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.each('SELECT id, txt FROM foo', function(err, row) {
        assert.ok(false, "this should not be called");
    }, function(err, num) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test Database#each prepare fail without completion handler'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.each('SELECT id, txt FROM foo', function(err, row) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
        else {
            assert.ok(false, 'this should not be called')
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test Database#get prepare fail with param binding'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.get('SELECT id, txt FROM foo WHERE id = ?', 1, function(err, row) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test Database#all prepare fail with param binding'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.all('SELECT id, txt FROM foo WHERE id = ?', 1, function(err, row) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test Database#run prepare fail with param binding'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.run('SELECT id, txt FROM foo WHERE id = ?', 1, function(err, row) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test Database#each prepare fail with param binding'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.each('SELECT id, txt FROM foo WHERE id = ?', 1, function(err, row) {
        assert.ok(false, "this should not be called");
    }, function(err, num) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test Database#each prepare fail with param binding without completion handler'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var completed = false;

    db.each('SELECT id, txt FROM foo WHERE id = ?', 1, function(err, row) {
        if (err) {
            assert.equal(err.message, 'SQLITE_ERROR: no such table: foo');
            assert.equal(err.errno, sqlite3.ERROR);
            assert.equal(err.code, 'SQLITE_ERROR');
            completed = true;
        }
        else {
            assert.ok(false, 'this should not be called')
        }
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};
