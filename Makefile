ast:
	./build/bin/cinder --emit-ast -o test ./tests/test.ci

semantic:
	./build/bin/cinder --test-semantic -o test ./tests/test.ci

test:
	./build/bin/cinder --compile -o test ./tests/test.ci

llvm:
	./build/bin/cinder --emit-llvm -o test.ll ./tests/test.ci