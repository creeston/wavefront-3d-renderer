#include <cairo.h>
#include <gtk/gtk.h>

#include "canvas_drawing.h"
#include "utils.h"

GdkPixbuf *pixel_buffer;
guchar *pixels;
int rowstride, n_channels;
int x_viewpoint_min, x_viewpoint_max, y_viewpoint_min, y_viewpoint_max;

void init_buffer(int canvas_width, int canvas_height)
{
    y_viewpoint_min = 0;
    x_viewpoint_min = 0;
    x_viewpoint_max = canvas_width;
    y_viewpoint_max = canvas_height;
    if (pixel_buffer)
    {
        g_object_unref(pixel_buffer);
    }

    pixel_buffer = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8, canvas_width, canvas_height);
    n_channels = gdk_pixbuf_get_n_channels(pixel_buffer);
    rowstride = gdk_pixbuf_get_rowstride(pixel_buffer);
    pixels = gdk_pixbuf_get_pixels(pixel_buffer);
}

gboolean draw_pixel(int x, int y, struct color color)
{
    if (pixels == NULL)
    {
        return FALSE;
    }

    guchar *p = pixels + y * rowstride + x * n_channels;
    p[0] = color.r;
    p[1] = color.g;
    p[2] = color.b;

    return TRUE;
}

void clear_buffer()
{
    fill_buffer(0x000000);
}

void fill_buffer(int color)
{
    gdk_pixbuf_fill(pixel_buffer, color);
}

void set_pixbuf_for_surface(cairo_t *surface)
{
    gdk_cairo_set_source_pixbuf(surface, pixel_buffer, 1, 1);
}

gboolean is_ready_to_draw()
{
    return pixel_buffer != NULL;
}