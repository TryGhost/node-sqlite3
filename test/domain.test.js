var sqlite3 = require('..');
var assert = require('assert');
var domain;

try {
  domain = require('domain');
} catch (e) {
  return;
}

function testThrowException(dom,done) {
  var expectedException = 'THIS IS AN EXPECTED EXCEPTION';
  var oldListeners;
  function handleExceptionListeners() {
    // Detatch existing exception listeners (of Mocha)
    oldListeners = process.listeners('uncaughtException').slice(0);
    oldListeners.forEach(function(fn) {
      process.removeListener('uncaughtException',fn);
    });
    // Attach exception listeners of this test.
    process.on('uncaughtException',processExceptionCallback);
    dom.on('error',domainExceptionCallback);
  }
  function unhandleExceptionListeners() {
    // Detach exception listeners of this test.
    process.removeListener('uncaughtException',processExceptionCallback);
    dom.removeListener('error',domainExceptionCallback);
    // Reattach the existing exception listeners (of Mocha)
    oldListeners.forEach(function(fn) {
      process.on('uncaughtException',fn);
    });
  }
  function processExceptionCallback(e) {
    unhandleExceptionListeners();
    done('Exception was not caught by domain:',e);
  }
  function domainExceptionCallback(e) {
    unhandleExceptionListeners();
    assert.equal(expectedException,e);

    done();
  }

  handleExceptionListeners();

  throw expectedException;
}

describe('domain',function() {
  describe('on database creation',function() {
    var dom;
    before(function() {
      dom = domain.create();
    });

    it('should work for open',function(done) {
      dom.run(function() {
        var db = new sqlite3.Database(':memory:',function() {
          assert.equal(process.domain, dom);
          testThrowException(dom,done);
        });
      });
    });
  });

  describe('on individual calls',function() {
    var db,dom;
    beforeEach(function(done) {
      dom = domain.create();
      db = new sqlite3.Database(':memory:',function() {
        done();
      });
    });

    function testFn(functionName) {
      it('should work for Database#'+functionName,function(done) {
        dom.run(function() {
          db[functionName]('select 0',function() {
            assert.equal(process.domain,dom);
            testThrowException(dom,done);
          });
        });
      });
    }

    testFn('run');
    testFn('exec');
    testFn('prepare');
    testFn('get');
    testFn('map');
    testFn('each');

    afterEach(function(done) {
      dom.dispose();
      done();
    });
  });
});

