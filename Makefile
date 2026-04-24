main: main.c
	gcc -Wall -Wextra -O2 main.c -o sdb

clean:
	rm ./build/*
