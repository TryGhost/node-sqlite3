cd deps/mpool-2.1.0/
make
cd ../..
node-waf clean
node-waf configure build
