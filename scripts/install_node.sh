# here we set up the node version on the fly based on the matrix value.
# This is done manually so that the build works the same on OS X
rm -rf ~/.nvm/ && git clone --depth 1 https://github.com/creationix/nvm.git ~/.nvm
source ~/.nvm/nvm.sh
nvm install $1
nvm use $1
node --version
npm --version
which node
