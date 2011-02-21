// Copyright (c) 2009, Eric Fredricksen <e@fredricksen.net>
// Copyright (c) 2010, Orlando Vazquez <ovazquez@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

var sqlite3 = module.exports = exports = require('./sqlite3_bindings');
var sys = require("sys");

// function noop(err) {
//     if (err) throw err;
// };

var Database = sqlite3.Database;
var Statement = sqlite3.Statement;


// Database#prepare(sql, [bind1, bind2, ...], [callback])
Database.prototype.prepare = function(sql) {
    var callback, params = Array.prototype.slice.call(arguments, 1);

    if (params.length && typeof params[params.length - 1] !== 'function') {
        params.push(undefined);
    }

    if (params.length > 1) {
        var statement = new Statement(this, sql);
        return statement.bind.apply(statement, params);
    } else {
        return new Statement(this, sql, params.pop());
    }
};

