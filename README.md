NAME
----

node-sqlite - Asynchronous SQLite3 driver for Node.js

SYNOPSIS
--------

High-level Driver
=================

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

Low-level Driver
================

The low-level bindings directly interface with the SQLite C API. The API
approximately matches the SQLite3 API when it makes sense. Some deviations
from the API have been made to improve performance.

    var sys    = require('sys'),
        sqlite = require('sqlite_bindings');

    var db = new sqlite.Database();

    // open the database for reading if file exists
    // create new database file if not

    db.open("lilponies.db", function () {
      var colour = 'pink';
      var sql = 'SELECT name FROM ponies WHERE hair_colour = $hair_colour and gemstones = ?';

      var ponies = [];

      // The prepare method will try to prefetch one row of results, so that
      // if there are no rows we can avoid having to make two trips into the
      // thread-pool.
      // If `statement` and didn't have any variable place-holders to bind
      // and doesn't evaluate to true, then it means the statement
      // executed successfully but returned no rows (think INSERT's).

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


DESCRIPTION
-----------

This distribution includes two SQLite version 3 drivers: a low level driver
written in C++ and a high level driver. The latter wraps the former to add
nicer API.

It's HTML5 WebDatabasish but much, much simpler.

At the moment, this library is the bare minimum necessary to get results out
of SQLite, but extending either driver should be quite straight forward.

This SQLite interface is incompatible with version 2.

SQLite's synchronous nature is fundamentally incompatible with a non-blocking
system such as Node. To get around this synchronous calls happen within Node's
libeio thread-pool, in a similar manner to how POSIX calls are currently made.
SQLite's serialized threading mode is used to make sure we use SQLite safely.
See http://www.sqlite.org/threadsafe.html for more info.

The author is aware that SQLite ships with an asynchronous interface. This
interface however lacks the necessary notification mechanism to alert the
caller when the SQLite call has completed. In other words, to be able to
support callbacks, we use the synchronous driver within a seperate thread.

SEE ALSO
--------

* http://sqlite.org/docs.html
* http://github.com/grumdrig/node-sqlite/

AUTHORS
-------

Orlando Vazquez [ovazquez@gmail.com]

Ryan Dahl [ry@tinyclouds.org]

THANKS
------

Many thanks to Eric Fredricksen for his synchronous driver on which this
driver was originally based.

* http://github.com/grumdrig/node-sqlite/
* http://grumdrig.com/node-sqlite/

LICENSE
-------

node-sqlite is BSD licensed.

(c) 2010 Orlando Vazquez
