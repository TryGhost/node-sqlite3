#!/usr/bin/env bash

set -u -e

if [[ ! -d ../.nvm ]]; then
    git clone https://github.com/creationix/nvm.git ../.nvm
fi
set +u
source ../.nvm/nvm.sh
nvm install 0.10
nvm use 0.10
set -u
node --version
npm --version

npm install nw-gyp

OLD_PATH="$PATH"

GYP_ARGS="--runtime=node-webkit --target=$NODE_WEBKIT"
if [[ $(uname -s) == 'Darwin' ]]; then
    GYP_ARGS="${GYP_ARGS} --target_arch=ia32"
fi

if [[ $(uname -s) == 'Darwin' ]]; then
    export NW_DOWNLOAD=node-webkit-v${NODE_WEBKIT}-osx-ia32
    wget http://dl.node-webkit.org/v${NODE_WEBKIT}/${NW_DOWNLOAD}.zip
    unzip -q ${NW_DOWNLOAD}.zip
    export PATH=$(pwd)/node-webkit.app/Contents/MacOS/:${PATH}
    # v0.10.0-rc1 unzips with extra folder
    export PATH=$(pwd)/${NW_DOWNLOAD}/node-webkit.app/Contents/MacOS/:${PATH}
else
    export NW_DOWNLOAD=node-webkit-v${NODE_WEBKIT}-linux-x64
    # for testing node-webkit, launch a virtual display
    export DISPLAY=:99.0
    sh -e /etc/init.d/xvfb start +extension RANDR
    wget http://dl.node-webkit.org/v${NODE_WEBKIT}/${NW_DOWNLOAD}.tar.gz
    tar xf ${NW_DOWNLOAD}.tar.gz
    export PATH=$(pwd)/${NW_DOWNLOAD}:${PATH}
fi

npm install --build-from-source ${GYP_ARGS}

# test the package
node-pre-gyp package testpackage ${GYP_ARGS}

PUBLISH_BINARY=false
if test "${COMMIT_MESSAGE#*'[publish binary]'}" != "$COMMIT_MESSAGE"; then
    node-pre-gyp publish ${GYP_ARGS}
    node-pre-gyp info ${GYP_ARGS}
    node-pre-gyp clean ${GYP_ARGS}
    make clean
    # now install from binary
    INSTALL_RESULT=$(npm install ${GYP_ARGS} --fallback-to-build=false > /dev/null)$? || true
    # if install returned non zero (errored) then we first unpublish and then call false so travis will bail at this line
    if [[ $INSTALL_RESULT != 0 ]]; then echo "returned $INSTALL_RESULT";node-pre-gyp unpublish ${GYP_ARGS};false; fi
    # If success then we arrive here so lets clean up
    node-pre-gyp clean ${GYP_ARGS}
fi

# restore PATH
export PATH="$OLD_PATH"
export GYP_ARGS
rm -rf ${NW_DOWNLOAD}