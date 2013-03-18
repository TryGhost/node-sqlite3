# NAME

node-sqlite3 - Asynchronous, non-blocking [SQLite3](http://sqlite.org/) bindings for [Node.js](https://github.com/joyent/node) 0.2-0.4 (versions 2.0.x), **0.6.13+ and 0.8.x** (versions 2.1.x).

It is also possible to use node-sqlite3 in GUI applications running on [node-webkit](https://github.com/rogerwang/node-webkit) engine if the latter is built on one of the above (supported) Node.js versions.



# USAGE

Before usage, the module has to be installed (for example, `npm install sqlite3` for Node.js). See the section “[Building](#building)” for details.

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

## Building for Node.js

Make sure you have the sources for `sqlite3` installed. Mac OS X ships with these by default. If you don't have them installed, install the `-dev` package with your package manager, e.g. `apt-get install libsqlite3-dev` for Debian/Ubuntu. Make sure that you have at least `libsqlite3` >= 3.6.

Bulding also requires node-gyp to be installed. You can do this with npm:

    npm install -g node-gyp

Your system has to meet node-gyp's requirements (for example, to have Python installed). See the corresponding section of the [node-gyp's page](https://github.com/TooTallNate/node-gyp#installation) for details.

To obtain and build node-sqlite3:

    git clone git://github.com/developmentseed/node-sqlite3.git
    cd node-sqlite3
    ./configure
    make

You can also use [`npm`](https://github.com/isaacs/npm) to download and install them:

    npm install sqlite3

## Building for node-webkit

Because of the different ABI in node-webkit, [nw-gyp](https://github.com/rogerwang/nw-gyp) has to be used instead of node-gyp. You may install nw-gyp with npm:

    npm install -g nw-gyp

Your system has to meet node-gyp's requirements (for example, to have Python installed). See the corresponding section of the [nw-gyp's page](https://github.com/rogerwang/nw-gyp#installation) for details.

To obtain and build node-sqlite3:

    git clone git://github.com/developmentseed/node-sqlite3.git
    cd node-sqlite3
    nw-gyp configure --target=0.4.2
    nw-gyp build

(The `--target` parameter must contain the target version of node-webkit.)

Keep in mind that you cannot use the short command `npm install sqlite3` to build for node-webkit (it builds for Node.js instead).



# TESTS

## Testing with Node.js

[expresso](https://github.com/visionmedia/expresso) and [step](https://github.com/creationix/step) are required to run unit tests.

    npm install --dev
    make test

## Testing with node-webkit

[step](https://github.com/creationix/step) is required to run unit tests.

The way of running expresso from within node-webkit has yet to be found. An alternative test wrapper for node-webkit is provided in the `test/node-webkit` subdirectory.

    npm install step@0.0.4
    nw test/node-webkit

The `test/node-webkit/index.html` file contains a list of delay intervals (in milliseconds).

```js
var ppDelay = 2000;
var largeDelay = 12000;
var extraLargeDelay = 70000;
```

They define how long the wrapper waits for a test to be completed. (Longer delays are intended for large and extra large tests.) You may tweak these according to your computer's speed. If a test doesn't complete in the given time, it is considered failed (unlike expresso).

The “Developer Tools” window is automatically opened. You may use its “Console” tab to see if some test fails. (In case of a failure, the subsequent tests are not run.)



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



# ACKNOWLEDGEMENTS

Thanks to [Orlando Vazquez](https://github.com/orlandov),
[Eric Fredricksen](https://github.com/grumdrig) and
[Ryan Dahl](https://github.com/ry) for their SQLite bindings for node, and to mraleph on Freenode's #v8 for answering questions.

Development of this module is sponsored by [Development Seed](http://developmentseed.org/).



# LICENSE

`node-sqlite3` is [BSD licensed](https://github.com/developmentseed/node-sqlite3/raw/master/LICENSE).
