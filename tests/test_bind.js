var sqlite = require('../sqlite3_bindings');
var sys = require('sys');
assert = require('assert');

var puts = sys.puts;
var inspect = sys.inspect;

function test_by_index(callback) {
  db = new sqlite.Database();
  db.open(":memory:", function () {
    puts("Opened ok");
    db.prepare("SELECT ? AS foo, ? AS bar, ? AS baz"
              , function (error, statement) {
      if (error) throw error;
      puts("Prepared ok");
      statement.bind(1, "hi", function (error) {
        if (error) throw error;
        puts("bound ok");
        statement.bind(2, "there", function (error) {
          if (error) throw error;
          puts("bound ok");
          statement.bind(3, "world", function (error) {
            if (error) throw error;
            puts("bound ok");
            statement.step(function (error, row) {
              if (error) throw error;
              assert.deepEqual(row, { foo: 'hi', bar: 'there', baz: 'world' });
            });
          });
        });
      });
    });
  });
}

function test_num_array(callback) {
  db = new sqlite.Database();
  db.open(":memory:", function () {
    puts("Opened ok");
    db.prepare("SELECT CAST(? AS INTEGER) AS foo, CAST(? AS INTEGER) AS bar, CAST(? AS INTEGER) AS baz"
              , function (error, statement) {
      if (error) throw error;
      puts("Prepared ok");
      statement.bindArray([1, 2, 3], function (error) {
        statement.step(function (error, row) {
          if (error) throw error;
          assert.deepEqual(row, { foo: 1, bar: 2, baz: 3 });
        });
      });
    });
  });
}

function test_by_array(callback) {
  db = new sqlite.Database();
  db.open(":memory:", function () {
    puts("Opened ok");
    db.prepare("SELECT ? AS foo, ? AS bar, ? AS baz"
              , function (error, statement) {
      if (error) throw error;
      puts("Prepared ok");
      statement.bindArray(['hi', 'there', 'world'], function (error) {
        if (error) throw error;
        puts("bound ok");
        statement.step(function (error, row) {
          assert.deepEqual(row, { foo: 'hi', bar: 'there', baz: 'world' });
          statement.reset();
          statement.bindArray([1, 2, null], function (error) {
            statement.step(function (error, row) {
              if (error) throw error;
              assert.deepEqual(row, { foo: 1, bar: 2, baz: null });
            });
          });
        });
      });
    });
  });
}

test_num_array();
test_by_index();
test_by_array();
