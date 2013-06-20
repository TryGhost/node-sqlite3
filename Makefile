build:
	node-gyp build

clean:
	rm -f test/support/big.db*
	rm -f test/tmp/*
	rm -rf ./deps/sqlite-autoconf-*/
	rm -rf ./build
	rm -rf ./out

db:
	@if ! [ -f test/support/big.db ]; then                                   \
		echo "Creating test database... This may take several minutes." ;      \
		node test/support/createdb.js ;                                        \
	else                                                                     \
		echo "okay: database already created" ;                                \
	fi

test: db
	npm test

.PHONY: build clean test
