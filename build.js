#!/usr/bin/env node

/*
TODO
 - verbose/quiet mode
 - travis/nvm/32bit auto-build and post to s3 for linux
 - cloudfront + logging
 - script to check for acl-public
 - use require() to support node_modules location of binary?
 - consider json config for configuring build and for handling routing remotely
 - drop tar.gz - use node-tar directly - https://github.com/isaacs/node-tar/issues/11
*/

var package_json = require('./package.json');
var Binary = require('./lib/binary_name.js').Binary;
var util = require('./build-util/tools.js');
var mkdirp = require('mkdirp');
var targz = require('tar.gz');
var cp = require('child_process');
var fs = require('fs');
var path = require('path');
var os = require('os');
var crypto = require('crypto');

var opts = {
    name: 'node_sqlite3',
    force: false,
    stage: false,
    configuration: 'Release',
    target_arch: process.arch,
    platform: process.platform,
    uri: 'http://node-sqlite3.s3.amazonaws.com/',
    tool: 'node-gyp',
    paths: {}
}

function log(msg) {
    console.log('['+package_json.name+']: ' + msg);
}

// only for dev
function log_debug(msg) {
    //log(msg);
}

function done(err) {
    if (err) {
        log(err);
        process.exit(1);
    }
    process.exit(0);
}

function test(opts,try_build,callback) {
    fs.statSync(opts.paths.runtime_module_path);
    var args = [];
    var shell_cmd;
    var arch_names = {
        'ia32':'-i386',
        'x64':'-x86_64'
    }
    if (process.platform === 'darwin' && arch_names[opts.target_arch]) {
        shell_cmd = 'arch';
        args.push(arch_names[opts.target_arch]);
        args.push(process.execPath);
    } else if (process.arch == opts.target_arch) {
        shell_cmd = process.execPath;
    }
    if (!shell_cmd) {
        // system we cannot test on - likely since we are cross compiling
        log("Skipping testing binary for " + process.target_arch);
        return callback();
    }
    args.push('lib/sqlite3');
    cp.execFile(shell_cmd, args, function(err, stdout, stderr) {
        if (err || stderr) {
            var output = err.message || stderr;
            log('Testing the binary failed: "' + output + '"');
            if (try_build) {
                log('Attempting source compile...');
                build(opts,callback);
            }
        } else {
            log('Sweet: "' + opts.binary.filename() + '" is valid, node-sqlite3 is now installed!');
            return callback();
        }
    });
}

function build(opts,callback) {
    var shell_cmd = opts.tool;
    if (opts.tool == 'node-gyp' && process.platform === 'win32') {
        shell_cmd = 'node-gyp.cmd';
    }
    var shell_args = ['rebuild'].concat(opts.args);
    var cmd = cp.spawn(shell_cmd,shell_args, {cwd: undefined, env: process.env, customFds: [ 0, 1, 2]});
    cmd.on('error', function (err) {
        if (err) {
            return callback(new Error("Failed to execute '" + shell_cmd + ' ' + shell_args.join(' ') + "' (" + err + ")"));
        }
    });
    // exit not close to support node v0.6.x
    cmd.on('exit', function (code) {
        if (code !== 0) {
            return callback(new Error("Failed to execute '" + shell_cmd + ' ' + shell_args.join(' ') + "' (" + code + ")"));
        }
        move(opts,callback);
    });
}

function tarball(opts,callback) {
    var source = path.dirname(opts.paths.staged_module_file_name);
    log('Compressing: ' + source + ' to ' + opts.paths.tarball_path);
    new targz(9).compress(source, opts.paths.tarball_path, function(err) {
        if (err) return callback(err);
        log('Versioned binary staged for upload at ' + opts.paths.tarball_path);
        var sha1 = crypto.createHash('sha1');
        fs.readFile(opts.paths.tarball_path,function(err,buffer) {
            if (err) return callback(err);
            sha1.update(buffer);
            log('Writing shasum at ' + opts.paths.tarball_shasum);
            fs.writeFile(opts.paths.tarball_shasum,sha1.digest('hex'),callback);
        });
    });
}

function move(opts,callback) {
    try {
        fs.statSync(opts.paths.build_module_path);
    } catch (ex) {
        return callback(new Error('Build succeeded but target not found at ' + opts.paths.build_module_path));
    }
    try {
        mkdirp.sync(path.dirname(opts.paths.runtime_module_path));
        log('Created: ' + path.dirname(opts.paths.runtime_module_path));
    } catch (err) {
        log_debug(err);
    }
    fs.renameSync(opts.paths.build_module_path,opts.paths.runtime_module_path);
    if (opts.stage) {
        try {
            mkdirp.sync(path.dirname(opts.paths.staged_module_file_name));
            log('Created: ' + path.dirname(opts.paths.staged_module_file_name))
        } catch (err) {
            log_debug(err);
        }
        fs.writeFileSync(opts.paths.staged_module_file_name,fs.readFileSync(opts.paths.runtime_module_path));
        // drop build metadata into build folder
        var metapath = path.join(path.dirname(opts.paths.staged_module_file_name),'build-info.json');
        // more build info
        opts.date = new Date();
        opts.node_features = process.features;
        opts.versions = process.versions;
        opts.config = process.config;
        opts.execPath = process.execPath;
        fs.writeFileSync(metapath,JSON.stringify(opts,null,2));
        tarball(opts,callback);
    } else {
        log('Installed in ' + opts.paths.runtime_module_path + '');
        test(opts,false,callback);
    }
}

function rel(p) {
    return path.relative(process.cwd(),p);
}

var opts = util.parse_args(process.argv.slice(2),opts);
opts.binary = new Binary(opts);
var versioned = opts.binary.getRequirePath();
opts.paths.runtime_module_path = rel(path.join(__dirname, 'lib', versioned));
opts.paths.runtime_folder = rel(path.join(__dirname, 'lib', 'binding',opts.binary.configuration));
var staged_module_path = path.join(__dirname, 'stage', opts.binary.getModuleAbi(), opts.binary.getBasePath());
opts.paths.staged_module_file_name = rel(path.join(staged_module_path,opts.binary.filename()));
opts.paths.build_module_path = rel(path.join(__dirname, 'build', opts.binary.configuration, opts.binary.filename()));
opts.paths.tarball_path = rel(path.join(__dirname, 'stage', opts.binary.configuration, opts.binary.getArchivePath()));
opts.paths.tarball_shasum = opts.paths.tarball_path.replace(opts.binary.compression(),'.sha1.txt');

if (!{ia32: true, x64: true, arm: true}.hasOwnProperty(opts.target_arch)) {
    return done(new Error('Unsupported (?) architecture: '+ opts.target_arch+ ''));
}

if (opts.force) {
    build(opts,done);
} else {
    try {
        test(opts,true,done);
    } catch (ex) {
        var from = opts.binary.getRemotePath();
        var tmpdirbase = '/tmp/';
        if (process.env.npm_config_tmp) {
            tmpdirbase = process.env.npm_config_tmp
        } else if (os.tmpdir) {
            tmpdirbase = os.tmpdir();
        }
        var tmpdir = path.join(tmpdirbase,'node-sqlite3-'+opts.binary.configuration);
        try {
            mkdirp.sync(tmpdir);
        } catch (err) {
            log_debug(err);
        }

        log('Checking for ' + from);
        util.download(from,{progress:false}, function(err,buffer) {
            if (err) {
                log(from + ' not found, falling back to source compile (' + err + ')');
                return build(opts,done);
            }
            // calculate shasum of tarball
            var sha1 = crypto.createHash('sha1');
            sha1.update(buffer);
            var actual_shasum = sha1.digest('hex');

            // write local tarball now to make debugging easier if following checks fail
            var tmpfile = path.join(tmpdir,path.basename(from));
            fs.writeFile(tmpfile,buffer,function(err) {
                if (err) return done(err);
                log('Downloaded to: '+ tmpfile);
                // fetch shasum expected value
                var from_shasum = from.replace(opts.binary.compression(),'.sha1.txt');
                log('Checking for ' + from_shasum);
                util.download(from_shasum,{progress:false},function(err,expected_shasum_buffer) {
                    if (err) {
                        log(from_shasum + ' not found, skipping shasum check (' + err + ')');
                        return done();
                    } else {
                        // now check shasum match
                        var expected = expected_shasum_buffer.toString().trim();
                        if (expected !== actual_shasum) {
                            return done(new Error("shasum does not match between remote and local: " + expected + ' ' + actual_shasum));
                        } else {
                            log('Sha1sum matches! ' + expected);
                            // we are good: continue
                            log('Extracting to ' + opts.paths.runtime_folder);
                            new targz().extract(tmpfile, opts.paths.runtime_folder, function(err) {
                                if (err) return done(err);
                                try {
                                    return test(opts,true,done);
                                } catch (ex) {
                                    // Stat failed
                                    log(opts.paths.runtime_folder + ' not found, falling back to source compile');
                                    return build(opts,done);
                                }
                            });
                        }
                    }
                });
            });
        });
    }
}
