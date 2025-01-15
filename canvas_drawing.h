#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <math.h>
#include <cairo.h>

struct color
{
    int r, g, b;
};

gboolean draw_pixel(int x, int y, struct color clr);
void clear_buffer();
void initialize_canvas_buffer(int canvas_width, int canvas_height);
void fill_buffer(int color);
void set_pixbuf_for_surface(cairo_t *surface);
gboolean is_ready_to_draw();
extern int x_viewpoint_min, x_viewpoint_max, y_viewpoint_min, y_viewpoint_max;
