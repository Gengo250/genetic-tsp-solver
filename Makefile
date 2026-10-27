CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Iinclude
LDFLAGS = -lraylib -lm
SRC = src/main.c src/gui.c src/point_generator.c src/population.c src/genetic.c
BIN = tsp

# Testes: sem Raylib, com AddressSanitizer + UndefinedBehaviorSanitizer.
TEST_SRC = tests/test_population.c src/point_generator.c src/population.c src/genetic.c
TEST_BIN = test_population
TEST_CFLAGS = -std=c17 -Wall -Wextra -g -Iinclude -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

run: $(BIN)
	./$(BIN)

test: $(TEST_SRC)
	$(CC) $(TEST_CFLAGS) $(TEST_SRC) -o $(TEST_BIN) -lm
	./$(TEST_BIN)

clean:
	rm -f $(BIN) $(TEST_BIN)

.PHONY: all run test clean
