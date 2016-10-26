all:
	gcc -rdynamic -g -std=c99 `pkg-config --cflags gtk+-3.0` -o ex0 main.c gtk_gui.c canvas_drawing.c renderer.c -lm -export-dynamic  `pkg-config --libs gtk+-3.0`
