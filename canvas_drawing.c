#include "canvas_drawing.h"

GdkPixbuf* buff;
guchar *pixels;
int rowstride, n_channels;
int Xvp_min, Xvp_max, Yvp_min, Yvp_max;

void swapI(int *x, int *y) {
    int aux;
    aux = *x;
    *x = *y;
    *y = aux;
}

void init_buffer(int canvas_width, int canvas_height) {
	Yvp_min = 0; Xvp_min = 0; Xvp_max = canvas_width; Yvp_max = canvas_height;
	buff = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8, canvas_width, canvas_height);
    n_channels = gdk_pixbuf_get_n_channels (buff);
    rowstride = gdk_pixbuf_get_rowstride (buff);
    pixels = gdk_pixbuf_get_pixels (buff);
}

void draw_pixel(int x, int y, struct color clr) {
    guchar* p = pixels + y * rowstride + x * n_channels;
    p[0] = clr.r;
    p[1] = clr.g;
    p[2] = clr.b;
}

void line(int x0, int y0, int x1, int y1, struct color Cl, int flag) {
    int steep = 0;
    if (abs(x0 - x1) < abs(y0 - y1)) {
        swapI(&x0, &y0);
        swapI(&x1, &y1);
        steep = 1;
    }
    if (x0 > x1) {
        swapI(&x0, &x1);
        swapI(&y0, &y1);
    }

    for (int x = x0; x <= x1; ++x) {
        float t = (x - x0) / (float)(x1 - x0);
        int y = y0 * (1. - t) + y1 * t;
        if (flag == CROP)
      if (steep && y > Xvp_min && y < Xvp_max && x > Yvp_min && x < Yvp_max)
                draw_pixel(y, x, Cl);
            else if (x > Xvp_min && x < Xvp_max && y > Yvp_min && y < Yvp_max)
                draw_pixel(x, y, Cl);
        if (flag == NOCROP) 
      if (steep)
               draw_pixel(y, x, Cl);
            else 
               draw_pixel(x, y, Cl);
    }
}

void clear_buffer() {
	gdk_pixbuf_fill(buff, 0x000000);
}