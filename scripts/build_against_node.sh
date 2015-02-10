#!/usr/bin/env bash

set -u -e

function publish() {
    if test "${COMMIT_MESSAGE#*'[publish binary]'}" != "$COMMIT_MESSAGE"; then
        node-pre-gyp publish
        node-pre-gyp info
        node-pre-gyp clean
        make clean
        # now install from binary
        INSTALL_RESULT=$(npm install --fallback-to-build=false > /dev/null)$? || true
        # if install returned non zero (errored) then we first unpublish and then call false so travis will bail at this line
        if [[ $INSTALL_RESULT != 0 ]]; then echo "returned $INSTALL_RESULT";node-pre-gyp unpublish;false; fi
        # If success then we arrive here so lets clean up
        node-pre-gyp clean
    fi
}

if [[ ! -d ../.nvm ]]; then
    git clone https://github.com/creationix/nvm.git ../.nvm
fi
set +u
source ../.nvm/nvm.sh
nvm install $NODE_VERSION
nvm use $NODE_VERSION
set -u

# test installing from source
npm install --build-from-source
node-pre-gyp package testpackage
npm test

publish

# now test building against shared sqlite
if [[ $(uname -s) == 'Darwin' ]]; then
    brew install sqlite
    npm install --build-from-source --sqlite=$(brew --prefix)
else
    sudo apt-get -qq update
    sudo apt-get -qq install libsqlite3-dev
    npm install --build-from-source --sqlite=/usr
fi
npm test

platform=$(uname -s | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/")

if [[ $(uname -s) == 'Linux' ]]; then
    sudo apt-get -y install gcc-multilib g++-multilib
    # node v0.8 and above provide pre-built 32 bit and 64 bit binaries
    # so here we use the 32 bit ones to also test 32 bit builds
    NVER=`node -v`
    # enable 32 bit node
    export PATH=$(pwd)/node-${NVER}-${platform}-x86/bin:$PATH
    if [[ ${NODE_VERSION:0:4} == 'iojs' ]]; then
        wget http://iojs.org/dist/${NVER}/iojs-${NVER}-${platform}-x86.tar.gz
        tar xf iojs-${NVER}-${platform}-x86.tar.gz
        # enable 32 bit iojs
        export PATH=$(pwd)/iojs-${NVER}-${platform}-x86/bin:$PATH
    else
        wget http://nodejs.org/dist/${NVER}/node-${NVER}-${platform}-x86.tar.gz
        tar xf node-${NVER}-${platform}-x86.tar.gz
        # enable 32 bit node
        export PATH=$(pwd)/node-${NVER}-${platform}-x86/bin:$PATH
    fi
    # install 32 bit compiler toolchain and X11
    # test source compile in 32 bit mode with internal libsqlite3
    CC=gcc-4.6 CXX=g++-4.6 npm install --build-from-source
    node-pre-gyp package testpackage
    npm test
    publish
    make clean
    # test source compile in 32 bit mode against external libsqlite3
    sudo apt-get -y install libsqlite3-dev:i386
    CC=gcc-4.6 CXX=g++-4.6 npm install --build-from-source --sqlite=/usr
    npm test
fi
