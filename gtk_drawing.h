#include <cairo.h>

void set_pixbuf_for_surface(cairo_t *surface);
void initialize_canvas_buffer(int canvas_width, int canvas_height);

extern int x_viewpoint_min, x_viewpoint_max, y_viewpoint_min, y_viewpoint_max;
