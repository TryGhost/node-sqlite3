Asynchronous, non-blocking [SQLite3](http://sqlite.org/) bindings for [Node.js](http://nodejs.org/).

[![NPM](https://nodei.co/npm/sqlite3.png?downloads=true&downloadRank=true)](https://nodei.co/npm/sqlite3/)

[![Build Status](https://travis-ci.org/mapbox/node-sqlite3.svg?branch=master)](https://travis-ci.org/mapbox/node-sqlite3)
[![Build status](https://ci.appveyor.com/api/projects/status/gvm7ul0hpmdawqom)](https://ci.appveyor.com/project/Mapbox/node-sqlite3)
[![Dependencies](https://david-dm.org/mapbox/node-sqlite3.svg)](https://david-dm.org/mapbox/node-sqlite3)


## Supported platforms

The `sqlite3` module works with Node.js v0.10.x or v0.12.x and io.js v1.x or 2.x.

Binaries for most Node versions and platforms are provided by default via [node-pre-gyp](https://github.com/mapbox/node-pre-gyp).

The `sqlite3` module also works with [node-webkit](https://github.com/rogerwang/node-webkit) if node-webkit contains a supported version of Node.js engine. [(See below.)](#building-for-node-webkit)

SQLite's [SQLCipher extension](https://github.com/sqlcipher/sqlcipher) is also supported. [(See below.)](#building-for-sqlcipher)

# Usage

**Note:** the module must be [installed](#installing) before use.

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

# Features

 - Straightforward query and parameter binding interface
 - Full Buffer/Blob support
 - Extensive [debugging support](https://github.com/mapbox/node-sqlite3/wiki/Debugging)
 - [Query serialization](https://github.com/mapbox/node-sqlite3/wiki/Control-Flow) API
 - [Extension support](https://github.com/mapbox/node-sqlite3/wiki/Extensions)
 - Big test suite
 - Written in modern C++ and tested for memory leaks


# API

See the [API documentation](https://github.com/mapbox/node-sqlite3/wiki) in the wiki.


# Installing

You can use [`npm`](https://github.com/isaacs/npm) to download and install:

* The latest `sqlite3` package: `npm install sqlite3`

* GitHub's `master` branch: `npm install https://github.com/mapbox/node-sqlite3/tarball/master`

In both cases the module is automatically built with npm's internal version of `node-gyp`,
and thus your system must meet [node-gyp's requirements](https://github.com/TooTallNate/node-gyp#installation).

It is also possible to make your own build of `sqlite3` from its source instead of its npm package ([see below](#building-from-the-source)).

It is possible to use the installed package in [node-webkit](https://github.com/rogerwang/node-webkit) instead of the vanilla Node.js. See [Building for node-webkit](#building-for-node-webkit) for details.

## Source install

Unless building via `npm install` (which uses its own `node-gyp`) you will need `node-gyp` installed globally:

    npm install node-gyp -g

The sqlite3 module depends only on libsqlite3. However, by default, an internal/bundled copy of sqlite will be built and statically linked, so an externally installed sqlite3 is not required.

If you wish to install against an external sqlite then you need to pass the `--sqlite` argument to `node-gyp`, `npm install` or the `configure` wrapper.

    ./configure --sqlite=/usr/local
    make

Or, using the node-gyp directly:

     node-gyp --sqlite=/usr/local
     make

Or, using npm:

     npm install --sqlite=/usr/local

If building against an external sqlite3 make sure to have the development headers available. Mac OS X ships with these by default. If you don't have them installed, install the `-dev` package with your package manager, e.g. `apt-get install libsqlite3-dev` for Debian/Ubuntu. Make sure that you have at least `libsqlite3` >= 3.6.

Note, if building against homebrew-installed sqlite on OS X you can do:

    ./configure --sqlite=/usr/local/opt/sqlite/
    make

## Building for node-webkit

Because of ABI differences, `sqlite3` must be built in a custom to be used with [node-webkit](https://github.com/rogerwang/node-webkit).

To build node-sqlite3 for node-webkit:

1. Install [`nw-gyp`](https://github.com/rogerwang/nw-gyp) globally: `npm install nw-gyp -g` *(unless already installed)*

2. Build the module with the custom flags of `--runtime`, `--target_arch`, and `--target`:

```sh
NODE_WEBKIT_VERSION="0.8.6" # see latest version at https://github.com/rogerwang/node-webkit#downloads
npm install sqlite3 --build-from-source --runtime=node-webkit --target_arch=ia32 --target=$(NODE_WEBKIT_VERSION)
```

This command internally calls out to [`node-pre-gyp`](https://github.com/mapbox/node-pre-gyp) which itself calls out to [`nw-gyp`](https://github.com/rogerwang/nw-gyp) when the `--runtime=node-webkit` option is passed.

You can also run this command from within a `node-sqlite3` checkout:

```sh
npm install --build-from-source --runtime=node-webkit --target_arch=ia32 --target=$(NODE_WEBKIT_VERSION)
```

Remember the following:

* You must provide the right `--target_arch` flag. `ia32` is needed to target 32bit node-webkit builds, while `x64` will target 64bit node-webkit builds (if available for your platform).

* After the `sqlite3` package is built for node-webkit it cannot run in the vanilla Node.js (and vice versa).
   * For example, `npm test` of the node-webkit's package would fail.

Visit the “[Using Node modules](https://github.com/rogerwang/node-webkit/wiki/Using-Node-modules)” article in the node-webkit's wiki for more details.

## Building for sqlcipher

For instructions for building sqlcipher see
[Building SQLCipher for node.js](https://coolaj86.com/articles/building-sqlcipher-for-node-js-on-raspberry-pi-2/)

To run node-sqlite3 against sqlcipher you need to compile from source by passing build options like:

    npm install sqlite3 --build-from-source --sqlite_libname=sqlcipher --sqlite=/usr/
    
    node -e 'require("sqlite3")'

If your sqlcipher is installed in a custom location (if you compiled and installed it yourself),
you'll also need to to set some environment variables:

### On OS X with Homebrew

Set the location where `brew` installed it:

    export LDFLAGS="-L`brew --prefix`/opt/sqlcipher/lib"
    export CPPFLAGS="-I`brew --prefix`/opt/sqlcipher/include"
    npm install sqlite3 --build-from-source --sqlite_libname=sqlcipher --sqlite=`brew --prefix`
    
    node -e 'require("sqlite3")'

### On most Linuxes (including Raspberry Pi)

Set the location where `make` installed it:

    export LDFLAGS="-L/usr/local/lib"
    export CPPFLAGS="-I/usr/local/include -I/usr/local/include/sqlcipher"
    export CXXFLAGS="$CPPFLAGS"
    npm install sqlite3 --build-from-source --sqlite_libname=sqlcipher --sqlite=/usr/local --verbose
    
    node -e 'require("sqlite3")'

# Testing

[mocha](https://github.com/visionmedia/mocha) is required to run unit tests.

In sqlite3's directory (where its `package.json` resides) run the following:

    npm install mocha
    npm test


# Contributors

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


# Acknowledgments

Thanks to [Orlando Vazquez](https://github.com/orlandov),
[Eric Fredricksen](https://github.com/grumdrig) and
[Ryan Dahl](https://github.com/ry) for their SQLite bindings for node, and to mraleph on Freenode's #v8 for answering questions.

Development of this module is sponsored by [MapBox](http://mapbox.org/).


# License

`node-sqlite3` is [BSD licensed](https://github.com/mapbox/node-sqlite3/raw/master/LICENSE).
