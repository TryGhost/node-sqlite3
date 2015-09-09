#!/usr/bin/env bash

source ~/.nvm/nvm.sh

set -e -u

function publish() {
    if [[ ${PUBLISHABLE:-false} == true ]] && [[ ${COMMIT_MESSAGE} =~ "[publish binary]" ]]; then
        node-pre-gyp package testpackage
        node-pre-gyp publish
        node-pre-gyp info
        make clean
    fi
}

# test installing from source
npm install --build-from-source  --clang=1
npm test

publish

# now test building against shared sqlite
if [[ $(uname -s) == 'Darwin' ]]; then
    brew install sqlite
    npm install --build-from-source --sqlite=$(brew --prefix) --clang=1
else
    npm install --build-from-source --sqlite=/usr --clang=1
fi
npm test

platform=$(uname -s | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/")

: '
if [[ $(uname -s) == 'Linux' ]]; then
    # node v0.8 and above provide pre-built 32 bit and 64 bit binaries
    # so here we use the 32 bit ones to also test 32 bit builds
    NVER=`node -v`
    # enable 32 bit node
    export PATH=$(pwd)/node-${NVER}-${platform}-x86/bin:$PATH
    if [[ ${NODE_VERSION:0:4} == 'iojs' ]]; then
        wget https://iojs.org/download/release/${NVER}/iojs-${NVER}-${platform}-x86.tar.gz
        tar xf iojs-${NVER}-${platform}-x86.tar.gz
        # enable 32 bit iojs
        export PATH=$(pwd)/iojs-${NVER}-${platform}-x86/bin:$(pwd)/iojs-${NVER}-${platform}-ia32/bin:$PATH
    else
        wget http://nodejs.org/dist/${NVER}/node-${NVER}-${platform}-x86.tar.gz
        tar xf node-${NVER}-${platform}-x86.tar.gz
        # enable 32 bit node
        export PATH=$(pwd)/node-${NVER}-${platform}-x86/bin:$(pwd)/node-${NVER}-${platform}-ia32/bin:$PATH
    fi
    which node
    ls -l $(which node)
    #node -e "console.log(process.arch,process.execPath)"
    # install 32 bit compiler toolchain and X11
    # test source compile in 32 bit mode with internal libsqlite3
    CC=gcc-4.6 CXX=g++-4.6 npm install --build-from-source  --clang=1
    node-pre-gyp package testpackage
    npm test
    publish
    make clean
    # broken for some unknown reason against io.js
    if [[ ${NODE_VERSION:0:4} != 'iojs' ]]; then
        # test source compile in 32 bit mode against external libsqlite3
        CC=gcc-4.6 CXX=g++-4.6 npm install --build-from-source --sqlite=/usr  --clang=1
        npm test
    fi
fi

'
