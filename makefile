CC = gcc
EMCC = emcc
CFLAGS = -g -std=c99
GTK_CFLAGS = `pkg-config --cflags gtk+-3.0`
SDL_CFLAGS = `sdl2-config --cflags`
GTK_LDFLAGS = `pkg-config --libs gtk+-3.0` -lm -rdynamic 
SDL_LDFLAGS = `sdl2-config --libs` -lm -rdynamic
SRCS_COMMON = src/wavefront_object_reader.c src/renderer.c src/utils.c
INCLUDES = -Iinclude

# GTK target
GTK_SRCS = src/main.c src/gtk_gui.c src/gtk_drawing.c $(SRCS_COMMON)
GTK_TARGET = app-gtk

# SDL target
SDL_SRCS = src/main_sdl.c src/sdl_gui.c src/sdl_drawing.c $(SRCS_COMMON)
SDL_TARGET = app-sdl

# SDL WASM target
SDL_WASM_TARGET = app-web
SDL_WASM_SRCS = src/main_wasm.c src/sdl_gui.c src/sdl_drawing.c $(SRCS_COMMON)
SDL_WASM_FLAGS = -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s USE_SDL_NET=2 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s FORCE_FILESYSTEM=1 -s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$$ccall' -s EXPORTED_FUNCTIONS='["_main", "_file_loaded", "_load_file", "_on_window_resize", "_init_ui"]'

all: $(GTK_TARGET) $(SDL_TARGET) $(SDL_WASM_TARGET)

$(GTK_TARGET): $(GTK_SRCS:.c=.o) resources.o
	$(CC) -o $@ $^ $(GTK_LDFLAGS)

$(SDL_TARGET): $(SDL_SRCS:.c=.o)
	$(CC) -o $@ $^ $(SDL_LDFLAGS)

$(SDL_WASM_TARGET):
	$(EMCC) $(SDL_WASM_SRCS) -o docs/index.html $(SDL_WASM_FLAGS)
	cp minimal_index.html docs/index.html

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(INCLUDES) -c $< -o $@

# Compile the resource XML file into a C source file
resources.c: resources/resources.xml resources/gui.glade
	glib-compile-resources --target=$@ --generate-source $<

# Compile the resource C source file into an object file
resources.o: resources.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(GTK_TARGET) $(SDL_TARGET) *.o resources.c src/*.o docs/index.html docs/index.js docs/index.wasm

.PHONY: all clean
