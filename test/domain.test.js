var sqlite3 = require('sqlite3');
var assert = require('assert');
var shouldSkip = false;
var domain;

try {
  domain = require('domain');
} catch (e) {
  shouldSkip = true;
}


if(shouldSkip) {
  exports['skipping domain tests'] = function(beforeExit) {
    beforeExit(function() {

    });
  };
  return;
}

if (process.setMaxListeners) process.setMaxListeners(0);

var protect = function(fn) {
  return function(beforeExit) {
    var oldListeners = process.listeners('uncaughtException').splice(0);

    oldListeners.filter(function(fn) {
      // XXX: This is a bit of a hack:
      // we know the name of the function that
      // the domain module adds as a listener
      // for `uncaughtException`, and we want
      // that to remain available -- but we
      // don't want expresso's uncaughtHandler
      // to be fired as well.
      return fn.name === 'uncaughtHandler'
    }).forEach(function(fn) {
      process.on('uncaughtException', fn)
    })

    var realBeforeExit;
    var injectBeforeExit = function(fn) {
          realBeforeExit = fn;
        };

    beforeExit(function() {
      var listeners = process.listeners('uncaughtException');

      // put expresso's uncaughtException handler
      // back in place.
      listeners.splice.apply(
        listeners, [0, listeners.length].concat(oldListeners)
      ); 

      // call the real `beforeExit` function defined
      // by the test, if any.
      if (realBeforeExit) {
        realBeforeExit();
      }
    })

    fn(injectBeforeExit);
  };
}

var testStatementMethod = function(method) {
  return function(beforeExit) {
    var dom = domain.create();
    var db = new sqlite3.Database(':memory:');
    var caughtCount = 0;
    var expected = 1 + ~~(Math.random() * 10);

    dom.on('error', function(err) {
      ++caughtCount;
    });

    dom.run(function() {
      db[method]('garbled nonsense', function(err) {
        // set a timeout, so that we ensure that the
        // domain chain gets passed along.

        for (var i = 0; i < expected; ++i) {
          setTimeout(function() {
            if (err) {
              throw err;
            }
          }, 0);
        }
      });
    });

    beforeExit(function() {
      assert.equal(caughtCount, expected);
    });
  };
}

exports['test Database#run works with domains'] = protect(testStatementMethod('run'));
exports['test Database#prepare works with domains'] = protect(testStatementMethod('prepare'));
exports['test Database#get works with domains'] = protect(testStatementMethod('get'));
exports['test Database#map works with domains'] = protect(testStatementMethod('all'));
exports['test Database#each works with domains'] = protect(testStatementMethod('each'));
exports['test Database#map works with domains'] = protect(testStatementMethod('map'));




