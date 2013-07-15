build:
	node-gyp build

clean:
	rm -f test/support/big.db*
	rm -f test/tmp/*
	rm -rf ./deps/sqlite-autoconf-*/
	rm -rf ./build
	rm -rf ./out

test:
	npm test

.PHONY: build clean test
