# Changlog

## 2.2.4

Released July 14th, 2014

 - Now supporting node v0.11.x (specifically >=0.11.13)
 - Fix db opening error with absolute path on windows
 - Updated to node-pre-gyp@0.5.18
 - updated internal libsqlite3 from 3.8.4.3 -> 3.8.5 (http://www.sqlite.org/news.html)

## 2.2.3

 - Fixed regression in v2.2.2 for installing from binaries on windows.

## 2.2.2

 - Fixed packaging problem whereby a `config.gypi` was unintentially packaged and could cause breakages for OS X builds.

## 2.2.1

 - Now shipping with 64bit FreeBSD binaries against both node v0.10.x and node v0.8.x.
 - Fixed solaris/smartos source compile by passing `-std=c99` when building internally bundled libsqlite3 (#201)
 - Reduced size of npm package by ignoring tests and examples.
 - Various fixes and improvements for building against node-webkit
 - Upgraded to node-pre-gyp@0.5.x from node-pre-gyp@0.2.5
 - Improved ability to build from source against `sqlcipher` by passing custom library name: `--sqlite_libname=sqlcipher`
 - No changes to C++ Core / Existing binaries are exactly the same

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


