CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Iinclude
LDFLAGS = -lraylib -lm
SRC = src/main.c src/gui.c src/point_generator.c
BIN = tsp

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: all run clean
