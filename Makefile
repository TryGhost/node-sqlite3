build:
	npm install

clean:
	rm test/support/big.db*
	rm test/tmp/*
	rm -rf ./deps/sqlite-autoconf-*/
	rm -rf ./build

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
