CC := $(if $(filter command-line,$(origin CC)),$(CC),$(shell which clang 2>/dev/null || which gcc 2>/dev/null))

main: main.c
	$(CC) -Wall -Werror -Wextra -pedantic -std=c17 -pthread -g main.c -o sdb

clean:
	rm sdb
