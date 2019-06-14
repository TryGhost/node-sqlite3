#https://www.gnu.org/prep/standards/html_node/Standard-Targets.html#Standard-Targets

all: build

./node_modules:
	npm install --build-from-source

build: ./node_modules
	./node_modules/.bin/node-pre-gyp build --loglevel=silent

debug:
	./node_modules/.bin/node-pre-gyp rebuild --debug

verbose:
	./node_modules/.bin/node-pre-gyp rebuild --loglevel=verbose

clean:
	@rm -rf ./build
	rm -rf lib/binding/
	rm -f test/support/big.db-journal
	rm -rf ./node_modules/

grind:
	valgrind --leak-check=full node node_modules/.bin/_mocha

testpack:
	rm -f ./*tgz
	npm pack
	tar -ztvf *tgz
	rm -f ./*tgz

rebuild:
	@make clean
	@make

test:
	npm test

check: test

.PHONY: test clean build
