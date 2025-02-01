#include <SDL2/SDL.h>

#if WASM
#include <emscripten.h>
#endif

#include "../include/renderer.h"
#include "../include/gui.h"
#include "../include/sdl_drawing.h"
#include "../include/wavefront_object_reader.h"
#include "../include/utils.h"
#include "../include/sdl_wasm_gui.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

struct wavefront_obj object_to_render;

const struct color GREY = {200, 200, 200};
const struct color WHITE = {255, 255, 255};
const int BLACK_BACKGROUND = 0x00000000;

const int SCALE_STEP = 5;
const int MIN_SCALE_VALUE = 10;
const int MAX_SCALE_VALUE = 150;
const int DEFAULT_SCALE_VALUE = 50;
const int MAX_NUMBER_OF_TRIANGLES = 100000;

double current_x_alpha = 0;
double current_y_beta = 0;
double current_z_gamma = 0;
double current_scale_value = DEFAULT_SCALE_VALUE;

int is_left_mouse_pressed = 0;
int is_right_mouse_pressed = 0;
int previous_mouse_position_x = 0, previous_mouse_position_y = 0;

void update_rotation_angles()
{
    set_cartesian_translation(DEG_TO_RAD(current_x_alpha), DEG_TO_RAD(current_y_beta), DEG_TO_RAD(current_z_gamma));
}

void handle_window_event(SDL_Event *event)
{
    if (event->window.event == SDL_WINDOWEVENT_RESIZED)
    {
        int width = event->window.data1;
        int height = event->window.data2;
        SDL_RenderSetLogicalSize(renderer, width, height);
        initialize_canvas_buffer(renderer, width, height);
        initialize_renderer();
    }
}

void resize_canvas(int width, int height)
{
    SDL_SetWindowSize(window, width, height);
    SDL_RenderSetLogicalSize(renderer, width, height);
    initialize_canvas_buffer(renderer, width, height);
    initialize_renderer();
}

void show_warning_dialog(const char *message)
{
    int res = SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning", message, window);
    if (res < 0)
    {
        fprintf(stderr, "Warning dialog could not be shown: %s\n", SDL_GetError());
    }
}

void set_file(const char *url)
{
    struct wavefront_obj new_object;
    int read_result = read_object(url, &new_object);
    if (!read_result)
    {
#if WASM
        show_warning_dialog("The file could not be read.");
#else
        fprintf(stderr, "The file could not be read.\n");
#endif
        return;
    }

    if (new_object.number_of_vertices > MAX_NUMBER_OF_TRIANGLES)
    {
#if WASM
        show_warning_dialog("The object has too many triangles to render.");
#else
        fprintf(stderr, "The object has too many triangles to render.\n");
#endif
        return;
    }

    object_to_render = new_object;
    set_object_to_render(object_to_render);
    initialize_renderer();
}

void set_shading(enum shading_type shading_type_value)
{
    set_shading_type(shading_type_value);
}

int main_loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
#if WASM
            emscripten_cancel_main_loop();
#endif
            return 0;
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            handle_window_event(&event);
        }
        else if (event.type == SDL_MOUSEWHEEL)
        {
            if (event.wheel.y > 0) // scroll up
            {
                current_scale_value += SCALE_STEP;
            }
            else if (event.wheel.y < 0) // scroll down
            {
                current_scale_value -= SCALE_STEP;
            }

            current_scale_value = fmax(MIN_SCALE_VALUE, current_scale_value);
            current_scale_value = fmin(MAX_SCALE_VALUE, current_scale_value);

            double zoom = (double)(current_scale_value) / 100 - 0.5;
            set_scale(1 + (zoom * 2));
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                is_left_mouse_pressed = 1;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                is_right_mouse_pressed = 1;
            }
            previous_mouse_position_x = event.button.x;
            previous_mouse_position_y = event.button.y;
        }
        else if (event.type == SDL_MOUSEBUTTONUP)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                is_left_mouse_pressed = 0;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                is_right_mouse_pressed = 0;
            }
        }
        else if (event.type == SDL_MOUSEMOTION)
        {
            if (is_left_mouse_pressed || is_right_mouse_pressed)
            {
                int x = event.motion.x, y = event.motion.y;
                int dx = x - previous_mouse_position_x, dy = y - previous_mouse_position_y;
                previous_mouse_position_x = x;
                previous_mouse_position_y = y;

                if (is_left_mouse_pressed)
                {
                    current_x_alpha -= (double)dy / 4;
                    current_y_beta += (double)dx / 4;
                    update_rotation_angles();
                }
                else if (is_right_mouse_pressed)
                {
                    current_z_gamma += (double)dx / 4;
                    update_rotation_angles();
                }
            }
        }
    }

    if (object_to_render.number_of_vertices > 0)
    {
        render();
        SDL_RenderPresent(renderer);
    }

    return 1;
}

void init_gui(struct arguments arguments, int argc, char **argv)
{
#if !WASM
    if (!arguments.file)
    {
        fprintf(stderr, "No file provided.\n");
        return;
    }
#endif

    if (arguments.height <= 0 || arguments.width <= 0)
    {
        fprintf(stderr, "Invalid canvas dimensions.\n");
        return;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    SDL_DisplayMode display_mode;
    if (SDL_GetCurrentDisplayMode(0, &display_mode) != 0)
    {
        fprintf(stderr, "Could not get display mode: %s\n", SDL_GetError());
        return;
    }

    if (arguments.width > display_mode.w || arguments.height > display_mode.h)
    {
        fprintf(stderr, "Canvas dimensions are larger than the screen.\n");
        return;
    }

    window = SDL_CreateWindow("Wavefront 3D Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, arguments.width, arguments.height, SDL_WINDOW_OPENGL);
    if (!window)
    {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    initialize_canvas_buffer(renderer, arguments.width, arguments.height);
    set_colors(BLACK_BACKGROUND, WHITE, GREY);

    if (arguments.file)
    {
        struct wavefront_obj new_object;
        int read_result = read_object(arguments.file, &new_object);
        if (!read_result)
        {
            fprintf(stderr, "The file could not be read.\n");
            return;
        }

        object_to_render = new_object;

        if (object_to_render.number_of_vertices > MAX_NUMBER_OF_TRIANGLES)
        {
            fprintf(stderr, "The object has too many triangles to render.\n");
            return;
        }

        set_object_to_render(object_to_render);
        initialize_renderer();
    }

#if WASM
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    int should_continue = 1;
    while (should_continue)
    {
        should_continue = main_loop();
    }
#endif
}

void cleanup_gui()
{
    destroy_object(object_to_render);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}