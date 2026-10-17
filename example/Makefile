CC = gcc

CFLAGS = -Wall -Wextra -Wconversion -pedantic -g -fsanitize=address

SRC = example.c
OUT = example

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(OUT)

.PHONY: clean
