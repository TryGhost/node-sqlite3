NAME
----

node-sqlite - Asynchronous SQLite3 driver for Node.js

SYNOPSIS
--------

    var sys    = require('sys'),
        sqlite = require('./sqlite');

    var db = new sqlite.Database();

    // open the database for reading if file exists
    // create new database file if not

    db.open("lilponies.db", function () {
      var colour = 'pink';
      var sql = 'SELECT name FROM ponies WHERE tail_colour = ?';

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


DESCRIPTION
-----------

This distribution includes two SQLite version 3 drivers: a low level driver
written in C++ and a high level driver. The latter wraps the former to add
nicer API and per-database query queuing to ensure queries do not clobber each
other.

It's HTML5 WebDatabase'ish but much, much simpler.

At the moment, this library is the bare minimum necessary to get results out
of SQLite, but extending either driver should be quite straight forward.

This SQLite interface is incompatible with verison 2.

SQLite's synchronous nature is fundamentally incompatible with a non-blocking
system such as Node. To get around this synchronous calls happen within Node's
libeio thread-pool, in a similar manner to how POSIX calls are currently made.
This is exposed to JavaScript in the form of Database and Statement objects.

The author is aware that SQLite ships with an asynchronous interface. This
interface however lacks the necessariy notification mechanism to alert the
caller when the SQLite call has completed. In other words, to be able to
support callbacks, we use the synchronous driver within a seperate thread.

SEE ALSO
--------

* http://sqlite.org/docs.html
* http://github.com/grumdrig/node-sqlite/

AUTHOR
------

Orlando Vazquez [ovazquez@gmail.com]

THANKS
------

Many thanks to Eric Fredricksen for his synchronous driver on which this was
based.

http://github.com/grumdrig/node-sqlite/
http://grumdrig.com/node-sqlite/

(c) 2010 Eric Fredricksen
(c) 2010 Orlando Vazquez
