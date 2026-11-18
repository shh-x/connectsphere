CC = gcc
CFLAGS = -Wall -Wextra -std=c99
CPPFLAGS = -Iinclude

MODULE_SRCS = src/utils.c src/hash.c src/dsu.c src/avl.c src/user.c
TEST_BINARIES = tests/test_hash tests/test_dsu tests/test_avl

.PHONY: all tests clean

all:
	@echo "Implementation build target is added in the final integration phase"

tests: $(TEST_BINARIES)
	@./tests/test_hash
	@./tests/test_dsu
	@./tests/test_avl

tests/test_hash: tests/test_hash.c src/hash.c src/utils.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_hash.c src/hash.c src/utils.c

tests/test_dsu: tests/test_dsu.c src/dsu.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_dsu.c src/dsu.c

tests/test_avl: tests/test_avl.c src/avl.c src/utils.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/test_avl.c src/avl.c src/utils.c

clean:
	rm -f $(TEST_BINARIES)
