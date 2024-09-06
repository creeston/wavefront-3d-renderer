all:
	gcc -rdynamic -g -std=c99 `pkg-config --cflags gtk+-3.0` -o app main.c gtk_gui.c canvas_drawing.c renderer.c -lm  `pkg-config --libs gtk+-3.0`
