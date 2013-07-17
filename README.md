# NAME

node-sqlite3 - Asynchronous, non-blocking [SQLite3](http://sqlite.org/) bindings for [node.js](https://github.com/joyent/node) 0.2-0.4 (versions 2.0.x), **0.6.13+, 0.8.x, and 0.10.x** (versions 2.1.x).

[![Build Status](https://travis-ci.org/developmentseed/node-sqlite3.png?branch=master)](https://travis-ci.org/developmentseed/node-sqlite3)
[![npm package version](https://badge.fury.io/js/sqlite3.png)](https://npmjs.org/package/sqlite3)


# USAGE

Install with `npm install sqlite3`.

``` js
var sqlite3 = require('sqlite3').verbose();
var db = new sqlite3.Database(':memory:');

db.serialize(function() {
  db.run("CREATE TABLE lorem (info TEXT)");

  var stmt = db.prepare("INSERT INTO lorem VALUES (?)");
  for (var i = 0; i < 10; i++) {
      stmt.run("Ipsum " + i);
  }
  stmt.finalize();

  db.each("SELECT rowid AS id, info FROM lorem", function(err, row) {
      console.log(row.id + ": " + row.info);
  });
});

db.close();
```



# FEATURES

* Straightforward query and parameter binding interface
* Full Buffer/Blob support
* Extensive [debugging support](https://github.com/developmentseed/node-sqlite3/wiki/Debugging)
* [Query serialization](https://github.com/developmentseed/node-sqlite3/wiki/Control-Flow) API
* [Extension support](https://github.com/developmentseed/node-sqlite3/wiki/Extensions)
* Big test suite
* Written in modern C++ and tested for memory leaks



# API

See the [API documentation](https://github.com/developmentseed/node-sqlite3/wiki) in the wiki.


# BUILDING

Unless building via `npm install` you will need `node-gyp` installed globally:

    npm install node-gyp -g

The module depends only on libsqlite3. However, by default, an internal/bundled copy of sqlite will be build and linked, so an externally installed sqlite3 is not required.

If you wish to install against an external sqlite then you need to pass the `--sqlite` argument to node-gyp. You can do this like:

    ./configure --sqlite=/usr/local
    make

Or like this (using the node-gyp built into npm):

     node-gyp --sqlite=/usr/local
     make

If building against an external sqlite3 make sure to have the development headers available. Mac OS X ships with these by default. If you don't have them installed, install the `-dev` package with your package manager, e.g. `apt-get install libsqlite3-dev` for Debian/Ubuntu. Make sure that you have at least `libsqlite3` >= 3.6.

Note, if building against homebrew-installed sqlite on OS X you can do:

    ./configure --sqlite=/usr/local/opt/sqlite/
    make

To obtain and build the bindings:

    git clone git://github.com/developmentseed/node-sqlite3.git
    cd node-sqlite3
    npm install

You can also use [`npm`](https://github.com/isaacs/npm) to download and install them:

    npm install sqlite3


# TESTS

[mocha](https://github.com/visionmedia/mocha) is required to run unit tests.

    npm install mocha
    npm test



# CONTRIBUTORS

* [Konstantin Käfer](https://github.com/kkaefer)
* [Dane Springmeyer](https://github.com/springmeyer)
* [Will White](https://github.com/willwhite)
* [Orlando Vazquez](https://github.com/orlandov)
* [Artem Kustikov](https://github.com/artiz)
* [Eric Fredricksen](https://github.com/grumdrig)
* [John Wright](https://github.com/mrjjwright)
* [Ryan Dahl](https://github.com/ry)
* [Tom MacWright](https://github.com/tmcw)
* [Carter Thaxton](https://github.com/carter-thaxton)
* [Audrius Kažukauskas](https://github.com/audriusk)
* [Johannes Schauer](https://github.com/pyneo)
* [Mithgol](https://github.com/Mithgol)



# ACKNOWLEDGEMENTS

Thanks to [Orlando Vazquez](https://github.com/orlandov),
[Eric Fredricksen](https://github.com/grumdrig) and
[Ryan Dahl](https://github.com/ry) for their SQLite bindings for node, and to mraleph on Freenode's #v8 for answering questions.

Development of this module is sponsored by [Development Seed](http://developmentseed.org/).


# LICENSE

`node-sqlite3` is [BSD licensed](https://github.com/developmentseed/node-sqlite3/raw/master/LICENSE).
