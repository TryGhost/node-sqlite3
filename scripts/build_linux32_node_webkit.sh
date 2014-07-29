#!/usr/bin/env bash

# rebuild node-sqlite3 for 32 bit node-webkit target
GYP_ARGS="${GYP_ARGS} --target_arch=ia32"
node-pre-gyp rebuild ${GYP_ARGS}
# install 32 bit stuff necessary for node-webkit 32 bit
sudo apt-get -y install libx11-6:i386
sudo apt-get -y install libxtst6:i386
sudo apt-get -y install libcap2:i386
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
# download and unpack node-webkit, add it to PATH
wget http://dl.node-webkit.org/v${NODE_WEBKIT}/node-webkit-v${NODE_WEBKIT}-linux-ia32.tar.gz
tar xf node-webkit-v${NODE_WEBKIT}-linux-ia32.tar.gz
OLD_PATH="$PATH";
export PATH=$(pwd)/node-webkit-v${NODE_WEBKIT}-linux-ia32:$PATH
# ldd nw (helps to find out if some necessary apt-get line is missing)
ldd $(pwd)/node-webkit-v${NODE_WEBKIT}-linux-ia32/nw
# attempt node-pre-gyp package testpackage
node-pre-gyp package testpackage ${GYP_ARGS}
# publish if necessary
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
# erase used node-webkit, restore PATH
export PATH="$OLD_PATH"; rm -rf node-webkit-v${NODE_WEBKIT}-linux-ia32
