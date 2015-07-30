# Changelog

## 3.0.10

 - Upgraded SQLite to 3.8.11.1: https://www.sqlite.org/releaselog/3_8_11_1.html

## 3.0.9

 - Fixed build regression against alpine linux (musl)
 - Upgraded node-pre-gyp@0.6.8

## 3.0.8

 - Fixed build regression against FreeBSD
 - Upgraded node-pre-gyp@0.6.7

## 3.0.7

 - Fixed build regression against ARM and i386 linux
 - Upgraded node-pre-gyp@0.6.6
 - Added support for io.js 2.0.0

## 3.0.6

 - Upgraded node-pre-gyp@0.6.5
 - Upgraded nan@1.8.4
 - Fixed binaries to work on older linux systems (circa GLIBC_2.2.5 like centos 6) @bnoordhuis
 - Updated internal libsqlite3 from 3.8.7.1 -> 3.8.9 (http://www.sqlite.org/news.html)

## 3.0.5

 - IO.js and Node v0.12.x support.
 - Node-webkit v0.11.x support regressed in this release, sorry (https://github.com/mapbox/node-sqlite3/issues/404).

## 3.0.4

 - Upgraded node-pre-gyp@0.6.1

## 3.0.3

 - Upgraded to node-pre-gyp@0.6.0 which should fix crashes against node v0.11.14
 - Now providing binaries against Visual Studio 2014 (pass --toolset=v140) and use binaries from https://github.com/mapbox/node-cpp11

## 3.0.2

 - Republish for possibly busted npm package.

## 3.0.1

 - Use ~ in node-pre-gyp semver for more flexible dep management.

## 3.0.0

Released September 20nd, 2014

 - Backwards-incompatible change: node versions 0.8.x are no longer supported.
 - Updated to node-pre-gyp@0.5.27
 - Updated NAN to 1.3.0
 - Updated internal libsqlite3 to v3.8.6

## 2.2.7

Released August 6th, 2014

 - Removed usage of `npm ls` with `prepublish` target (which breaks node v0.8.x)

## 2.2.6

Released August 6th, 2014

 - Fix bundled version of node-pre-gyp

## 2.2.5

Released August 5th, 2014

 - Fix leak in complete() callback of Database.each() (#307)
 - Started using `engineStrict` and improved `engines` declaration to make clear only >= 0.11.13 is supported for the 0.11.x series.

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
