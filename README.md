# NAME

node-sqlite - Asynchronous SQLite3 driver for Node.js

SQLite calls block, so to work around this, synchronous calls happen within
Node's libeio thread-pool, in a similar manner to how POSIX calls are
currently made.

# SYNOPSIS

## High-level Driver

    var sys    = require('sys'),
        sqlite = require('sqlite');

    var db = new sqlite.Database();

    // open the database for reading if file exists
    // create new database file if not

    db.open("aquateen.db", function (error) {
      if (error) {
          console.log("Purple Alert! Aqua Teen Database unabled to be opened!"));
          throw error;
      }
      var sql = 'SELECT name FROM dudes WHERE type = ? AND age > ?';

      db.prepare(sql, function (error, statement) {
        if (error) throw error;

        // Fill in the placeholders
        statement.bindArray(['milkshake', 30], function () {

          statement.fetchAll(function (error, rows) {
            // ...
          });
        });
      });
    });

# API

## Database Objects

To create a new database object:
    var sqlite = require('sqlite');

    var db = sqlite.Database();

### database.open(filename, function (error) {})

Open a database handle to the database at the specified filename. If the file
does not exist the bindings will attempt to create it. The callback takes no
arguments.

A filename of ":memory:" may be used to create an in-memory database.

### database.close(function (error) {})

Close the database handle.

### database.query(sql, [bindings,] function (error, row) {})

Execute a SQL query, `sql`, with optional bindings `bindings` on the currently
opened database. The callback will be executed once per row returned, plus
once more with row set to undefined to indicate end of results.

### database.executeScript(SQL, function (error) {});

    db.executeScript
      (   "CREATE TABLE table1 (id, name);"
        + "CREATE TABLE table2 (id, age);"
        + "INSERT INTO table1 (1, 'Mister Shake');"
        + "INSER INTO table2 (1, 34);"
      , function (error) {
          if (error) throw error;
          // ...
        });

Execute multiple semi-colon separated SQL statements. Statements must take no
placeholders. Each statement will be executed with a single step() and then
reset. This is ideally suited to executing multiple DDL statements.

### database.prepare(SQL, [options,] function (error, statement) {})

Create a prepared statement from an SQL string. Prepared statements can be
used used to iterate over results and to avoid compiling SQL each time a query
is performed.

Options:

- lastInsertRowID: boolean, default false.
    If true, when this statement is step()'d over, the context object (this) in
    the callback will contain a lastInsertRowID member with the ID of the last
    inserted row.

- affectedRows: boolean, default false.
    If true, when this statement is step()'d over, the context object (this) in
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
represented as objects with properties named after the respective columns.

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
