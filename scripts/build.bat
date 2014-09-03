@rem setup
@rem git clone git://github.com/marcelklehr/nodist.git
@rem create ~/.node_pre_gyprc
@rem note, for 64 builds you may need to win7 sdk terminal
@rem https://github.com/TooTallNate/node-gyp/issues/112
set PATH=c:\dev2\nodist\bin;%PATH%
set PATH=node_modules\.bin;%PATH%
set PATH=%PATH%;c:\Python27

@rem 32 bit
set NODIST_X64=0
nodist use stable
node -e "console.log(process.version + ' ' + process.arch)"
node-pre-gyp clean
npm install --build-from-source
npm test
node-pre-gyp package publish
node-pre-gyp clean

@rem 64 bit
@ rem cannot open input file 'kernel32.lib' http://www.microsoft.com/en-us/download/details.aspx?id=4422
set NODIST_X64=1
nodist use stable
node -e "console.log(process.version + ' ' + process.arch)"
node-pre-gyp clean
npm install --build-from-source
npm test
node-pre-gyp package publish
node-pre-gyp clean
