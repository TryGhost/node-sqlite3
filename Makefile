
build:
	node-gyp build

clean:
	rm test/support/big.db*
	rm test/tmp/*
	node-gyp clean

db:
	@if ! [ -f test/support/big.db ]; then                                   \
		echo "Creating test database... This may take several minutes." ;      \
		node test/support/createdb.js ;                                        \
	else                                                                     \
		echo "okay: database already created" ;                                \
	fi

test: build db
	npm test

.PHONY: build clean test
