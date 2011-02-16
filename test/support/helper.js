var assert = require('assert');
var fs = require('fs');

exports.deleteFile = function(name) {
    try {
        fs.unlinkSync(name);
    } catch(err) {
        if (err.errno !== process.ENOENT) {
            throw err;
        }
    }
};

assert.fileDoesNotExist = function(name) {
    try {
        fs.statSync(name);
    } catch(err) {
        if (err.errno !== process.ENOENT) {
            throw err;
        }
    }
};

assert.fileExists = function(name) {
    try {
        fs.statSync(name);
    } catch(err) {
        throw err;
    }
};