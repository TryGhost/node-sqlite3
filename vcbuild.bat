del build.sln
rd /q /s Default
del lib\\node_sqlite3.node
python gyp/gyp build.gyp --depth=. -f msvs -G msvs_version=2010
msbuild build.sln
copy Default\\node_sqlite3.node lib\\node_sqlite3.node
rem test!
rem set NODE_PATH=lib
rem node node_modules\expresso\bin\expresso
rem node -e "console.log(require('eio'))"