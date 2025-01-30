CC = gcc
CFLAGS = -g -std=c99 `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0` -lm -rdynamic 
SRCS = src/main.c src/gtk_gui.c src/gtk_drawing.c src/renderer.c src/utils.c src/wavefront_object_reader.c
TARGET = app

all: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(SRCS:.c=.o) resources.o
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile the resource XML file into a C source file
resources.c: resources/resources.xml resources/gui.glade
	glib-compile-resources --target=$@ --generate-source $<

# Compile the resource C source file into an object file
resources.o: resources.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(TARGET) *.o resources.c

.PHONY: all clean
