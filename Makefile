OBJ = mxjson mxjson-tree mxjson-test mxjson-test-coverage
CFLAGS=-Wall -Wextra -Wpedantic -Wshadow -I. -O2
BIN=./bin

$(shell mkdir -p $(BIN))
TGTS = $(patsubst %,$(BIN)/%,$(OBJ))

all: $(TGTS)

test: $(BIN)/mxjson-test
	$^

coverage: $(BIN)/mxjson-test-coverage
	$^
	gcov -ar mxjson-test.c


$(BIN)/mxjson: examples/mxjson.c
	$(CC) $(CFLAGS) $^ -o $@

$(BIN)/mxjson-tree: examples/mxjson-tree.c
	$(CC) $(CFLAGS) $^ -o $@

$(BIN)/mxjson-test: test/mxjson-test.c
	$(CC) $(CFLAGS) $^ -o $@

$(BIN)/mxjson-test-coverage: test/mxjson-test.c
	$(CC) $(CFLAGS) --coverage $^ -o $@


clean:
	rm -f $(TGTS)
	rm -f *.gcda *.gcno *.gcov

.PHONY: all clean test coverage
