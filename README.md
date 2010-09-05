# NAME

node-sqlite - Asynchronous SQLite3 driver for Node.js

This distribution includes two SQLite libraries: a low level driver
written in C++ and a high level driver. The latter wraps the former to add
simpler API.

SQLite calls block, so to work around this, synchronous calls happen within
Node's libeio thread-pool, in a similar manner to how POSIX calls are
currently made. SQLite's serialized threading mode is used to make sure we
use SQLite safely. See http://www.sqlite.org/threadsafe.html for more info.

# SYNOPSIS

## High-level Driver

High-level bindings provide a simple interface to SQLite3. They should be
fast enough for most purposes, but if you absolutely need more performance,
the low level drivers are also straight-forward to use, but require a few
additional steps.

    var sys    = require('sys'),
        sqlite = require('sqlite');

    var db = new sqlite.Database();

    // open the database for reading if file exists
    // create new database file if not

    db.open("lilponies.db", function () {
      var colour = 'pink';
      var sql = 'SELECT name FROM ponies WHERE hair_colour = ?';

      // bindings list is optional

      var ponies = [];

      db.query(sql, [colour], function (pony) {
        if (!pony) {
          // no more ponies
          if (!ponies.length)
            sys.puts('There are no ponies with ' + colour + ' tails. :(');
          else
            sys.puts('The following ponies have ' + colour + ' tails: ' + ponies.join(', '));
        }
        sys.puts(sys.inspect(pony));
        ponies.push(pony);
      });
    });

## Low-level Driver

The low-level bindings directly interface with the SQLite C API. The API
approximately matches the SQLite3 API when it makes sense.

    var sys    = require('sys'),
        sqlite = require('sqlite/sqlite3_bindings');

    var db = new sqlite.Database();

    // open the database for reading if file exists
    // create new database file if not

    db.open("lilponies.db", function () {
      var colour = 'pink';
      var sql = 'SELECT name FROM ponies' +
                ' WHERE hair_colour = $hair_colour' +
                ' AND gemstones = ?';

      var ponies = [];

      db.prepare(sql, function (error, statement) {
        if (error) throw error;

        // Fill in the placeholders
        // Could also have used:
        //   statement.bind(position, value, function () { ... });
        //   statement.bindObject({ $hair_colour: 'pink' }, function () {});
        statement.bindArray(['pink', 4], function () {

          // call step once per row result
          statement.step(function (error, row) {
            if (!row) {
              // end of rows
            }

            // do some stuff
            // call statement.step() again for next row
          });
        });
      });
    });

# API

## Database Objects

To create a new database object:

    var db = sqlite_bindings.Database();

### database.open(filename, function (error) {})

Open a database handle to the database at the specified filename. If the file
does not exist the bindings will attempt to create it. The callback takes no
arguments.

A filename of ":memory:" may be used to create an in-memory database.

### database.close(function (error) {})

Close the database handle.

### database.prepare(SQL, [options,] function (error, statement) {})

Create a prepared statement from an SQL string. Prepared statements can be
used used to iterate over results and to avoid compiling SQL each time a query
is performed.  

Options:

- lastInsertRowID: boolean, default false.
    If true, when this statement is stepped over, the context object (this) in
    the callback will contain a lastInsertRowID member with the ID of the last
    inserted row.

- affectedRows: boolean, default false.
    If true, when this statement is stepped over, the context object (this) in
    the callback will contain an affectedRows member with the number of
    affected rows for the last step.

## Statement Objects

### statement.bindArray(array, function (error) {})

    statement.bindArray([1, 'robots', 4.20], callback)

Bind array items to place-holder values (? or $foo) in statement.

### statement.bindObject(object, function (error) {})

    statement.bindObject({ $name: 'meatwad',
                           $occupation: 'Former detective' }, callback)

Bind object properties to named place-holder values ($foo, $bar, $baz) in
statement.

### statement.bind(position, value, function (error) {})

    statement.bind(1, "tango", function (error) {})

Bind a value to a place-holder position. Because binding place-holders is done
by position (not index), the first place-holder is at position 1, second at
place-holder position 2, etc.

### statement.clearBindings()

Immediately clear the bindings from the statement. There is no callback.

### statement.step(function (error, row) {})

Fetch one row from a prepared statement and hand it off to a callback. If
there are no more rows to be fetched, row will be undefined. Rows are
represented as objects with
properties named after the respective columns.

### statement.fetchAll(function (error, rows) {})

Fetch all rows in statement and pass them to the callback as an array of
objects, each object representing one row.

### statement.reset()

Immediately reset a statement object back to it's initial state, ready to be
step() or fetchAll()'d again.

### statement.finalize(function (error) {})

Free SQLite objects associated with this statement and mark it for garbage
collection.

## Supported Types

At the moment, the supported types are TEXT, NUMBER, FLOAT and NULL.

# BUILDING

To obtain and build the bindings:

    git clone http://github.com/orlandov/node-sqlite.git
    cd node-sqlite
    node-waf configure build

# TESTS

Running the unit tests could not be easier. Simply:

    git submodule update --init
    ./run-tests

# SEE ALSO

* http://sqlite.org/docs.html
* http://github.com/grumdrig/node-sqlite/

# AUTHORS

Orlando Vazquez [ovazquez@gmail.com]

Ryan Dahl [ry@tinyclouds.org]

# THANKS

Many thanks to Eric Fredricksen for his synchronous driver on which this
driver was originally based.

* http://github.com/grumdrig/node-sqlite/
* http://grumdrig.com/node-sqlite/

# LICENSE

node-sqlite is BSD licensed.

(c) 2010 Orlando Vazquez
