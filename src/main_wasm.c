#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <emscripten.h>
#include "../include/gui.h"
#include "../include/models.h"
#include "../include/sdl_wasm_gui.h"

const char *argp_program_version = "wavefront-obj-renderer 1.0";
const char *argp_program_bug_address = "<mityy2012@gmail.com>";

void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s -f <file>\n", program_name);
}

void load_file(const char *url)
{
    EM_ASM({
        var url = UTF8ToString($0);
        var filename = url.split('/').pop();
        var xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.responseType = 'arraybuffer';
        xhr.onload = function() {
            if (xhr.status == 200) {
                var data = new Uint8Array(xhr.response);
                // check if file already exists
                if (FS.analyzePath('/' + filename).exists) {
                    FS.unlink('/' + filename);
                }
                FS.createDataFile('/', filename, data, true, true);
                ccall('file_loaded', 'void', ['string'], [filename]);
            } else {
                console.error('Failed to load file:', url);
            }
        };
        xhr.send(); }, url);
}

void file_loaded(char *filename)
{
    set_file(filename);
}

void init_ui(int width, int height)
{
    struct arguments arguments;
    arguments.width = width;
    arguments.height = height;
    init_gui(arguments, 0, NULL);
    cleanup_gui();
}

void on_window_resize(int width, int height)
{
    resize_canvas(width, height);
}

void set_shading_callback(char *shading_type_value)
{
    if (strcmp(shading_type_value, "flat") == 0)
    {
        set_shading(FLAT);
    }
    else if (strcmp(shading_type_value, "gouraud") == 0)
    {
        set_shading(GOURAUD);
    }
    else
    {
        set_shading(NO_SHADING);
    }
}

void set_camera_distance_callback(int distance)
{
    set_camera_distance_value(distance);
}

int main(int argc, char **argv)
{
    EM_ASM({
        var width = document.getElementById('canvas-container').clientWidth;
        var height = document.getElementById('canvas-container').clientHeight;
        ccall('init_ui', 'void', [ 'number', 'number' ], [ width, height ]);
    });

    return 0;
}
