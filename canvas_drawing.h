#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <math.h>

#define CROP 1
#define NOCROP 2

struct color {
    int r,g,b;
};

void line(int x0, int y0, int x1, int y1, struct color Cl, int flag);
void draw_pixel(int x, int y, struct color clr);
void clear_buffer();
void init_buffer(int canvas_width, int canvas_height);

extern int Xvp_min, Xvp_max, Yvp_min, Yvp_max;
extern GdkPixbuf *buff;