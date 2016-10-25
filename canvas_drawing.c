#include "canvas_drawing.h"

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