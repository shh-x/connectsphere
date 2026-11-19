CC = gcc
CFLAGS = -Wall -Wextra -std=c99
CPPFLAGS = -Iinclude

MODULE_SRCS = src/utils.c src/hash.c src/dsu.c src/avl.c src/user.c \
	src/graph.c src/heap.c src/mergesort.c src/storage.c src/perf.c
TEST_BINARIES = tests/test_hash tests/test_dsu tests/test_avl tests/test_heap \
	tests/test_mergesort tests/test_integration

.PHONY: all tests clean

all: bin/connectsphere bin/connectsphere_engine

bin:
	mkdir -p bin

bin/connectsphere: $(MODULE_SRCS) src/main.c src/cli_input.c | bin
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ src/main.c src/cli_input.c $(MODULE_SRCS)

bin/connectsphere_engine: $(MODULE_SRCS) src/engine_main.c src/json_protocol.c | bin
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ src/engine_main.c src/json_protocol.c $(MODULE_SRCS)

tests: $(TEST_BINARIES) bin/connectsphere_engine
	@./tests/test_hash
	@./tests/test_dsu
	@./tests/test_avl
	@./tests/test_heap
	@./tests/test_mergesort
	@./tests/test_integration
	@node tests/test_engine.js bin/connectsphere_engine
	@node tests/test_api.js

tests/test_hash: tests/test_hash.c src/hash.c src/utils.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_hash.c src/hash.c src/utils.c

tests/test_dsu: tests/test_dsu.c src/dsu.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_dsu.c src/dsu.c

tests/test_avl: tests/test_avl.c src/avl.c src/utils.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_avl.c src/avl.c src/utils.c

tests/test_heap: tests/test_heap.c src/heap.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_heap.c src/heap.c

tests/test_mergesort: tests/test_mergesort.c src/mergesort.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_mergesort.c src/mergesort.c

tests/test_integration: tests/test_integration.c src/user.c src/hash.c src/dsu.c \
	src/avl.c src/utils.c src/storage.c src/graph.c src/mergesort.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_integration.c src/user.c src/hash.c \
	src/dsu.c src/avl.c src/utils.c src/storage.c src/graph.c src/mergesort.c

clean:
	rm -f $(TEST_BINARIES) bin/connectsphere bin/connectsphere_engine \
		tests/integration_users.txt tests/integration_friendships.txt
