// Inspired by https://github.com/tlrobinson/long-stack-traces
const util = require('util');

function extendTrace(object, property) {
    const old = object[property];
    object[property] = function(...args) {
        return old.apply(this, args).catch((err) => {
            const error = new Error();
            const name = object.constructor.name + '#' + property + '(' +
                        Array.prototype.slice.call(arguments).map(function(el) {
                            return util.inspect(el, false, 0);
                        }).join(', ') + ')';
            if (err && err.stack && !err.__augmented) {
                err.stack = filter(err).join('\n');
                err.stack += '\n--> in ' + name;
                err.stack += '\n' + filter(error).slice(1).join('\n');
                err.__augmented = true;
            }
            throw err;
        });
    };
}
exports.extendTrace = extendTrace;


function filter(error) {
    return error.stack.split('\n').filter(function(line) {
        return line.indexOf(__filename) < 0;
    });
}
