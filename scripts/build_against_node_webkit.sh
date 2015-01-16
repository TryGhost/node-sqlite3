#!/usr/bin/env bash

set -u -e

if [[ ! -d ../.nvm ]]; then
    git clone https://github.com/creationix/nvm.git ../.nvm
fi
set +u
source ../.nvm/nvm.sh
nvm install 0.10
set -u

npm install nw-gyp -g

OLD_PATH="$PATH"

GYP_ARGS="--runtime=node-webkit --target=${NODE_WEBKIT} --target_arch=${TARGET_ARCH}"
if [[ $(uname -s) == 'Darwin' ]]; then
    if [[ '${TARGET_ARCH}' == 'x64' ]]; then
       # do not build on Mac OS X x64 until node-webkit 0.10.1 is released
       false
    fi
fi

if [[ $(uname -s) == 'Darwin' ]]; then
    export NW_DOWNLOAD=node-webkit-v${NODE_WEBKIT}-osx-${TARGET_ARCH}
    wget http://dl.node-webkit.org/v${NODE_WEBKIT}/${NW_DOWNLOAD}.zip
    unzip -q ${NW_DOWNLOAD}.zip
    export PATH=$(pwd)/node-webkit.app/Contents/MacOS/:${PATH}
    # v0.10.0-rc1 unzips with extra folder
    export PATH=$(pwd)/${NW_DOWNLOAD}/node-webkit.app/Contents/MacOS/:${PATH}
    npm install --build-from-source ${GYP_ARGS}
else
    sudo apt-get install build-essential
    # Linux
    export NW_DOWNLOAD=node-webkit-v${NODE_WEBKIT}-linux-${TARGET_ARCH}
    # for testing node-webkit, launch a virtual display
    export DISPLAY=:99.0
    # NOTE: travis already has xvfb installed
    # http://docs.travis-ci.com/user/gui-and-headless-browsers/#Using-xvfb-to-Run-Tests-That-Require-GUI-%28e.g.-a-Web-browser%29
    sh -e /etc/init.d/xvfb start +extension RANDR
    wget http://dl.node-webkit.org/v${NODE_WEBKIT}/${NW_DOWNLOAD}.tar.gz
    tar xf ${NW_DOWNLOAD}.tar.gz
    export PATH=$(pwd)/${NW_DOWNLOAD}:${PATH}
    if [[ "${TARGET_ARCH}" == 'ia32' ]]; then
        # for nw >= 0.11.0 on ia32 we need gcc/g++ 4.8
        IFS='.' read -a NODE_WEBKIT_VERSION <<< "${NODE_WEBKIT}"
        if test ${NODE_WEBKIT_VERSION[0]} -ge 0 -a ${NODE_WEBKIT_VERSION[1]} -ge 11; then
            # travis-ci runs ubuntu 12.04, so we need this ppa for gcc/g++ 4.8
            sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
            export CC=gcc-4.8
            export CXX=g++-4.8
            export CXXFLAGS="-fpermissive"
            COMPILER_PACKAGES="gcc-4.8-multilib g++-4.8-multilib"
        else
            export CC=gcc-4.6
            export CXX=g++-4.6
            export CXXFLAGS="-fpermissive"
            COMPILER_PACKAGES="gcc-multilib g++-multilib"
        fi
        # need to update to avoid 404 for linux-libc-dev_3.2.0-64.97_amd64.deb
        sudo apt-get update
        # prepare packages for 32-bit builds on Linux
        sudo apt-get -y install $COMPILER_PACKAGES libx11-6:i386 libnotify4:i386 libxtst6:i386 libcap2:i386 libglib2.0-0:i386 libgtk2.0-0:i386 libatk1.0-0:i386 libgdk-pixbuf2.0-0:i386 libcairo2:i386 libfreetype6:i386 libfontconfig1:i386 libxcomposite1:i386 libasound2:i386 libxdamage1:i386 libxext6:i386 libxfixes3:i386 libnss3:i386 libnspr4:i386 libgconf-2-4:i386 libexpat1:i386 libdbus-1-3:i386 libudev0:i386
        # also use ldd to find out if some necessary apt-get is missing
        ldd $(pwd)/${NW_DOWNLOAD}/nw
        npm install --build-from-source ${GYP_ARGS}
    else
        npm install --build-from-source ${GYP_ARGS}
    fi
fi

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
rm -rf ${NW_DOWNLOAD}
