#!/usr/bin/env bash

set -u -e

if [[ ! -d ../.nvm ]]; then
    git clone https://github.com/creationix/nvm.git ../.nvm
fi
source ../.nvm/nvm.sh
nvm install 0.10
nvm use 0.10
node --version
npm --version

# for testing node-webkit, launch a virtual display
export DISPLAY=:99.0
sh -e /etc/init.d/xvfb start +extension RANDR
npm install nw-gyp
node-pre-gyp rebuild --runtime=node-webkit --target=$NODE_WEBKIT
# now we need node-webkit itself to test
OLD_PATH="$PATH"
if [[ $(uname -s) == 'Darwin' ]]; then
    export NW_DOWNLOAD=node-webkit-v${NODE_WEBKIT}-osx-ia32
    wget http://dl.node-webkit.org/v${NODE_WEBKIT}/${NW_DOWNLOAD}.tar.gz
    unzip ${NW_DOWNLOAD}.zip
    export PATH=$(pwd)/${NW_DOWNLOAD}:$PATH
else
    export NW_DOWNLOAD=node-webkit-v${NODE_WEBKIT}-linux-x64
    wget http://dl.node-webkit.org/v${NODE_WEBKIT}/${NW_DOWNLOAD}.tar.gz
    tar xf ${NW_DOWNLOAD}.tar.gz
    export PATH=$(pwd)/${NW_DOWNLOAD}:$PATH
fi

# test the package
node-pre-gyp package testpackage --runtime=node-webkit --target=$NODE_WEBKIT

PUBLISH_BINARY=false
if test "${COMMIT_MESSAGE#*'[publish binary]'}" != "$COMMIT_MESSAGE"; then
    node-pre-gyp publish --runtime=node-webkit --target=$NODE_WEBKIT
    node-pre-gyp info --runtime=node-webkit --target=$NODE_WEBKIT
    node-pre-gyp clean --runtime=node-webkit --target=$NODE_WEBKIT
    make clean
    # now install from binary
    INSTALL_RESULT=$(npm install --runtime=node-webkit --target=$NODE_WEBKIT --fallback-to-build=false > /dev/null)$? || true
    # if install returned non zero (errored) then we first unpublish and then call false so travis will bail at this line
    if [[ $INSTALL_RESULT != 0 ]]; then echo "returned $INSTALL_RESULT";node-pre-gyp unpublish --runtime=node-webkit --target=$NODE_WEBKIT;false; fi
    # If success then we arrive here so lets clean up
    node-pre-gyp clean --runtime=node-webkit --target=$NODE_WEBKIT
fi

# restore PATH
export PATH="$OLD_PATH"
rm -rf ${NW_DOWNLOAD}

# TODO linux 32 bit
: '
# rebuild node-sqlite3 for 32 bit node-webkit target (if NODE_WEBKIT is not empty)
node-pre-gyp rebuild --runtime=node-webkit --target=$NODE_WEBKIT
# on Linux 32 bit: install 32 bit stuff necessary for node-webkit 32 bit
sudo apt-get -y install libx11-6:i386
sudo apt-get -y install libglib2.0-0:i386
sudo apt-get -y install libgtk2.0-0:i386
sudo apt-get -y install libatk1.0-0:i386
sudo apt-get -y install libgdk-pixbuf2.0-0:i386
sudo apt-get -y install libcairo2:i386
sudo apt-get -y install libfreetype6:i386
sudo apt-get -y install libfontconfig1:i386
sudo apt-get -y install libxcomposite1:i386
sudo apt-get -y install libasound2:i386
sudo apt-get -y install libxdamage1:i386
sudo apt-get -y install libxext6:i386
sudo apt-get -y install libxfixes3:i386
sudo apt-get -y install libnss3:i386
sudo apt-get -y install libnspr4:i386
sudo apt-get -y install libgconf-2-4:i386
sudo apt-get -y install libexpat1:i386
sudo apt-get -y install libdbus-1-3:i386
sudo apt-get -y install libudev0:i386
# on Linux 32 bit: download and unpack node-webkit, add it to PATH
wget http://dl.node-webkit.org/v${NODE_WEBKIT}/node-webkit-v${NODE_WEBKIT}-linux-ia32.tar.gz
tar xf node-webkit-v${NODE_WEBKIT}-linux-ia32.tar.gz
OLD_PATH="$PATH";
export PATH=$(pwd)/node-webkit-v${NODE_WEBKIT}-linux-ia32:$PATH
# on Linux 32 bit: ldd nw
ldd $(pwd)/node-webkit-v${NODE_WEBKIT}-linux-ia32/nw
# attempt node-pre-gyp package testpackage (if NODE_WEBKIT is not empty)
node-pre-gyp package testpackage publish --runtime=node-webkit --target=$NODE_WEBKIT
# on Linux 32 bit: erase used node-webkit, restore PATH
export PATH="$OLD_PATH"; rm -rf node-webkit-v${NODE_WEBKIT}-linux-ia32
'