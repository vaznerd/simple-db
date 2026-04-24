main: main.c
	gcc -Wall -Wextra -O2 main.c -o ./build/sdb

run: main
	./build/sdb

clean:
	rm ./build/*
