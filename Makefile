ast:
	./build/bin/cinder --emit-ast -o test test.ci

semantic:
	./build/bin/cinder --test-semantic -o test test.ci

test:
	./build/bin/cinder --compile -o test test.ci