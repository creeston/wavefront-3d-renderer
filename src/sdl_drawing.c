#include <SDL2/SDL.h>
#include "../include/drawing.h"
#include "../include/sdl_drawing.h"

SDL_Renderer *current_renderer;

int x_viewpoint_min, x_viewpoint_max, y_viewpoint_min, y_viewpoint_max;

void draw_pixel(int x, int y, int r, int g, int b)
{
    // Implementation for drawing a pixel using SDL
    SDL_SetRenderDrawColor(current_renderer, r, g, b, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawPoint(current_renderer, x, y);
}

void fill_buffer(int color)
{
    // Implementation for filling the buffer with a color using SDL
    SDL_SetRenderDrawColor(current_renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(current_renderer);
}

void initialize_canvas_buffer(SDL_Renderer *renderer, int canvas_width, int canvas_height)
{
    current_renderer = renderer;
    x_viewpoint_min = 0;
    x_viewpoint_max = canvas_width;
    y_viewpoint_min = 0;
    y_viewpoint_max = canvas_height;
}