#!/usr/bin/env bash

# source ~/.nvm/nvm.sh

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
if [[ ${COVERAGE} == true ]]; then
    CXXFLAGS="--coverage" LDFLAGS="--coverage" npm install --build-from-source  --clang=1
    npm test
    ./py-local/bin/cpp-coveralls --exclude node_modules --exclude tests --build-root build --gcov-options '\-lp' --exclude docs --exclude build/Release/obj/gen --exclude deps  > /dev/null
else
    echo "building binaries for publishing"
    CFLAGS="${CFLAGS:-} -include $(pwd)/src/gcc-preinclude.h" CXXFLAGS="${CXXFLAGS:-} -include $(pwd)/src/gcc-preinclude.h" V=1 npm install --build-from-source --clang=1 --verbose
    nm lib/binding/*/node_sqlite3.node | grep "GLIBCXX_" | c++filt  || true
    nm lib/binding/*/node_sqlite3.node | grep "GLIBC_" | c++filt || true
    npm test
fi


publish

# now test building against shared sqlite
echo "building from source to test against external libsqlite3"
export NODE_SQLITE3_JSON1=no
if [[ $(uname -s) == 'Darwin' ]]; then
    # brew install sqlite
    # already installed on trusty osx
    npm install --build-from-source --sqlite=$(brew --prefix) --clang=1 --verbose
else
    npm install --build-from-source --sqlite=/usr --clang=1
fi
npm test
