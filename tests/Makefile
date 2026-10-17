CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wconversion -pedantic -g -fsanitize=address,undefined -I..

SRC = test.c
HDR = ../my_vector.h
OUT = test

$(OUT): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $< -o $@

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)

.PHONY: run clean
