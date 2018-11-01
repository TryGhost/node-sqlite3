Asynchronous, non-blocking [SQLite3](http://sqlite.org/) bindings for [Node.js](http://nodejs.org/).

Extended version

## Supported platforms

The `sqlite3` module works with Node.js v4.x, v6.x, v8.x, and v10.x.

The `sqlite3` module also works with [node-webkit](https://github.com/rogerwang/node-webkit) if node-webkit contains a supported version of Node.js engine. [(See below.)](#building-for-node-webkit)

SQLite's [SQLCipher extension](https://github.com/sqlcipher/sqlcipher) is also supported. [(See below.)](#building-for-sqlcipher)

# Usage

**Note:** the module must be [installed](#installing) before use.

```javascript
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

# Custom functions

First implementation by [Whitney Young](https://github.com/wbyoung)

```javascript
    var sqlite3 = require('sqlite3').verbose();
    var db = new sqlite3.Database(':memory:', function() {
          db.registerFunction('A_PLUS_B', function(a, b) {
              return a+b;
          });
          
          db.all('SELECT A_PLUS_B(1, 2) AS val', function(err, rows) {
              console.log(row.id + ": " + row.info); //Prints [{val: 3}]
          });
          
          db.close();
    });
    

```

# Custom aggregate

```javascript
    var sqlite3 = require('sqlite3').verbose();
    var db = new sqlite3.Database(':memory:', function() {
          let tempStr = '';
          db.registerAggregateFunction('CUSTOM_AGGREGATE', function(value) {
              if(!value){
                  return tempStr;
              }
              tempStr+= value;
          });
          
          db.all('SELECT CUSTOM_AGGREGATE(id) AS val', function(err, rows) {
              console.log(row.id + ": " + row.info); //Prints [{val: '123456'}] if table has 6 rows with id field
          });
          
          db.close();
    });
    

```

# Features

 - Straightforward query and parameter binding interface
 - Full Buffer/Blob support
 - Extensive debugging support
 - Query serialization API
 - Extension support
 - Big test suite
 - Written in modern C++ and tested for memory leaks
 - Bundles Sqlite3 3.15.0 as a fallback if the installing system doesn't include SQLite
 - Custom functions. Implemented `sqlite_create_function` 

# API

See the [API documentation](https://github.com/lailune/node-sqlite3/wiki) in the wiki.

# Installing

You can use [`npm`](https://github.com/isaacs/npm) to download and install:

* GitHub's `master` branch: `npm install https://github.com/lailune/node-sqlite3/tarball/master`

It is possible to use the installed package in [node-webkit](https://github.com/rogerwang/node-webkit) instead of the vanilla Node.js. 


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
* [Whitney Young](https://github.com/wbyoung)
* [Andrey Nedobylsky](https://github.com/lailune)

# Acknowledgments

Originally developed by [MapBox](https://github.com/mapbox)
