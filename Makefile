ast:
	./build/bin/cinder --emit-ast -o test ./tests/test.ci

semantic:
	./build/bin/cinder --test-semantic -o test ./tests/test.ci

test:
	./build/bin/cinder --compile -o test ./tests/test.ci

llvm:
	./build/bin/cinder --emit-llvm -o test.ll ./tests/test.ci

# The -l="foo" syntax seems to be broken in cxxopts, the = is parsed as a command
# line option for whatever reason
linker:
	./build/bin/cinder --compile \
	-o test \
	-l "-Wall,-Wextra" \
	./tests/test.ci