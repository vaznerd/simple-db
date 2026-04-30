main: main.c
	clang -Wall -Werror -Wextra -pedantic -std=c17 -pthread -g main.c -o sdb

clean:
	rm ./build/*
