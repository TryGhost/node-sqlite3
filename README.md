# NAME

node-sqlite3 - Asynchronous, non-blocking [SQLite3](http://sqlite.org/) bindings for [node.js](https://github.com/ry/node) 0.2.\* and 0.4.\*.



# USAGE

Install with `npm install sqlite3`.

    var sqlite3 = require('sqlite3').verbose();
    var db = new sqlite3.Database(':memory:');

    db.serialize(function() {
      db.run("CREATE TABLE lorem (info TEXT)");

      var stmt = db.prepare("INSERT INTO lorem VALUES (?)");
      for (var i = 0; i < 10; i++) {
          stmt.run("Ipsum " + i);
      }

      db.each("SELECT rowid AS id, info FROM lorem", function(err, row) {
          console.log(row.id + ": " + row.info);
      });
    });

    db.close();



# API

See the [API documentation](https://github.com/developmentseed/node-sqlite3/wiki) in the wiki.


# BUILDING

Make sure you have the sources for `sqlite3` installed. Mac OS X ships with these by default. If you don't have them installed, install the `-dev` package with your package manager, e.g. `apt-get install libsqlite3-dev` for Debian/Ubuntu.

To obtain and build the bindings:

    git clone git://github.com/developmentseed/node-sqlite3.git
    cd node-sqlite3
    ./configure
    make

You can also use [`npm`](https://github.com/isaacs/npm) to download and install them:

    npm install sqlite3



# TESTS

[expresso](https://github.com/visionmedia/expresso) is required to run unit tests.

    npm install expresso
    make test



# CONTRIBUTORS

* [Konstantin Käfer](https://github.com/kkaefer)



# ACKNOWLEDGEMENTS

Thanks to [Orlando Vazquez](https://github.com/orlandov),
[Eric Fredricksen](https://github.com/grumdrig) and
[Ryan Dahl](https://github.com/ry) for their SQLite bindings for node, and to mraleph on Freenode's #v8 for answering questions.



# LICENSE

`node-sqlite3` is [BSD licensed](https://github.com/developmentseed/node-sqlite3/raw/master/LICENSE).
