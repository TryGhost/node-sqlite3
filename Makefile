
build:
	node-gyp build

clean:
	node-gyp clean

db:
	@if ! [ -f test/support/big.db ]; then                                   \
		echo "Creating test database... This may take several minutes." ;      \
		node test/support/createdb.js ;                                        \
	fi

ifndef only
test: build db
	@rm -rf ./test/tmp && mkdir -p ./test/tmp
	NODE_PATH=./lib:$NODE_PATH expresso -I lib test/*.test.js
else
test: build db
	@rm -rf ./test/tmp && mkdir -p ./test/tmp
	NODE_PATH=./lib:$NODE_PATH expresso -I lib test/${only}.test.js
endif

.PHONY: build clean test
