all:
	gcc -rdynamic -g -std=c99 `pkg-config --cflags gtk+-3.0` -o ex0 main.c -lm -export-dynamic  `pkg-config --libs gtk+-3.0`
