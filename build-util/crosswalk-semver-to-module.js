var https = require("https");
var http = require("http");
var fs   = require("fs");
var url = require('url');
var semver = require('semver');

var cross = {};

// https://github.com/developmentseed/node-sqlite3/wiki/Binaries

var template = 'https://raw.github.com/joyent/node/v{VERSION}/src/';

var sortObjectByKey = function(obj){
    var keys = [];
    var sorted_obj = {};
    for(var key in obj){
        if(obj.hasOwnProperty(key)){
            keys.push(key);
        }
    }
    // sort keys
    keys.sort(function(a,b) {
      if (semver.gt(a, b)) {
        return 1
      }
      return -1;
    });
    len = keys.length;

    for (i = 0; i < len; i++)
    {
      key = keys[i];
      sorted_obj[key] = obj[key];
    }
    return sorted_obj;
};

function get(ver,callback) {
  var header = 'node.h';
  if (semver.gt(ver, 'v0.11.4')) {
    // https://github.com/joyent/node/commit/44ed42bd971d58b294222d983cfe2908e021fb5d#src/node_version.h
    header = 'node_version.h';
  }
  var path = template.replace('{VERSION}',ver) + header;
  var uri = url.parse(path);
  https.get(uri, function(res) {
      if (res.statusCode != 200) {
        throw new Error("server returned " + res.statusCode + ' for: ' + path);
      }
      res.setEncoding('utf8');
      var body = '';
      res.on('data', function (chunk) {
        body += chunk;
      });
      res.on('end',function(err) {
        var term = 'define NODE_MODULE_VERSION'
        var idx = body.indexOf(term);
        var following = body.slice(idx);
        var end = following.indexOf('\n');
        var value = following.slice(term.length,end).trim();
        if (value[0] === '(' && value[value.length-1] == ')') {
          value = value.slice(1,value.length-1);
        } else if (value.indexOf(' ') > -1) {
          value = value.slice(0,value.indexOf(' '));
        }
        var int_val = +value;
        cross[ver] = int_val;
        return callback(null,ver,int_val);
      })
    });
}

process.on('exit', function(err) {
    var sorted = sortObjectByKey(cross);
    console.log(sorted);
})

var versions_doc = 'http://nodejs.org/dist/npm-versions.txt';
http.get(url.parse(versions_doc), function(res) {
    if (res.statusCode != 200) {
      throw new Error("server returned " + res.statusCode + ' for: ' + versions_doc);
    }
    res.setEncoding('utf8');
    var body = '';
    res.on('data', function (chunk) {
      body += chunk;
    });
    res.on('end',function(err) {
      var lines = body.split('\n').map(function(line) { return line.split(' ')[0].slice(1); }).filter(function(line) { return (line.length && line != 'node'); });
      lines.forEach(function(ver) {
          get(ver,function(err,version,result) {
            cross[version] = result;
          });
      });
    });
});