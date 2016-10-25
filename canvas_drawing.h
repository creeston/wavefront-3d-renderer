#include "helper.h"
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define CROP 1
#define NOCROP 2

void line(int x0, int y0, int x1, int y1, struct color Cl, int flag);
void draw_pixel(int x, int y, struct color clr);
void clear_surface (void);

cairo_surface_t *surface;
GdkPixbuf* buff;
guchar *pixels;
int canvas_height, canvas_width;
int rowstride, n_channels;
int Xvp_min, Xvp_max, Yvp_min, Yvp_max;