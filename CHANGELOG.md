# Changlog

## Future

 - Fixed solaris/smartos compile by passing `-std=c99` when building internally bundled libsqlite3 (#201)

## 2.2.0

Released Jan 13th, 2014

 - updated internal libsqlite3 from 3.7.17 -> 3.8.2 (http://www.sqlite.org/news.html) which includes the next-generation query planner http://www.sqlite.org/queryplanner-ng.html
 - improved binary deploy system using https://github.com/springmeyer/node-pre-gyp
 - binary install now supports http proxies
 - source compile now supports freebsd
 - fixed support for node-webkit

## 2.1.19

Released October 31st, 2013

 - Started respecting `process.env.npm_config_tmp` as location to download binaries
 - Removed uneeded `progress` dependency

## 2.1.18

Released October 22nd, 2013

 - `node-sqlite3` moved to mapbox github group
 - Fixed reporting of node-gyp errors
 - Fixed support for node v0.6.x

## 2.1.17
 - Minor fixes to binary deployment

## 2.1.16
 - Support for binary deployment

## 2.1.15

Released August 7th, 2013

 - Minor readme additions and code optimizations


