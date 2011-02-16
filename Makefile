
build:
	node-waf build

clean:
	node-waf clean

test: build
	expresso -I lib test/*.test.js

.PHONY: build clean test