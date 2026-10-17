CC ?= cc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Werror -g
SANITIZE = -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: test example clean

test: build/test_my_vector build/test_hooks
	./build/test_my_vector
	./build/test_hooks

build/%: tests/%.c my_vector.h
	mkdir -p build
	$(CC) $(CFLAGS) $(SANITIZE) $< -o $@

example: example/example.c my_vector.h
	mkdir -p build
	$(CC) $(CFLAGS) $< -o build/example
	./build/example

clean:
	rm -rf build
