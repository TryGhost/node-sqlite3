var assert = require('assert');
var fs = require('fs');
var pathExists = require('fs').exists || require('path').exists;

exports.deleteFile = function(name) {
    try {
        fs.unlinkSync(name);
    } catch(err) {
        if (err.errno !== process.ENOENT && err.code !== 'ENOENT' && err.syscall !== 'unlink') {
            throw err;
        }
    }
};

exports.ensureExists = function(name,cb) {
    pathExists(name,function(exists) {
        if (!exists) {
            fs.mkdir(name,function(err) {
                return cb(err);
            });
        }
        return cb(null);
    });
}

assert.fileDoesNotExist = function(name) {
    try {
        fs.statSync(name);
    } catch(err) {
        if (err.errno !== process.ENOENT && err.code !== 'ENOENT' && err.syscall !== 'unlink') {
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