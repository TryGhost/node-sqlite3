#!/usr/bin/env bash

source ~/.nvm/nvm.sh

set -e -u

export DISPLAY=":99.0"
GYP_ARGS="--runtime=electron --target=${ELECTRON_VERSION} --dist-url=https://atom.io/download/electron"
NPM_BIN_DIR="$(npm bin -g 2>/dev/null)"

function publish() {
    if [[ ${PUBLISHABLE:-false} == true ]] && [[ ${COMMIT_MESSAGE} =~ "[publish binary]" ]]; then
        node-pre-gyp package $GYP_ARGS
        node-pre-gyp publish $GYP_ARGS
        node-pre-gyp info $GYP_ARGS
    fi
}

function electron_pretest() {
    npm install -g electron@${ELECTRON_VERSION}
    npm install -g electron-mocha
    sh -e /etc/init.d/xvfb start
    sleep 3
}

function electron_test() {
    "$NPM_BIN_DIR"/electron test/support/createdb-electron.js
    "$NPM_BIN_DIR"/electron-mocha -R spec --timeout 480000
}

# test installing from source
npm install --build-from-source  --clang=1 $GYP_ARGS

electron_pretest
electron_test

publish
make clean

# now test building against shared sqlite
export NODE_SQLITE3_JSON1=no
if [[ $(uname -s) == 'Darwin' ]]; then
    brew install sqlite
    npm install --build-from-source --sqlite=$(brew --prefix) --clang=1 $GYP_ARGS
else
    npm install --build-from-source --sqlite=/usr --clang=1 $GYP_ARGS
fi
electron_test
export NODE_SQLITE3_JSON1=yes
