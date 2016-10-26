#include "gtk_gui.h"

#include <gtk/gtk.h>
#include "renderer.h"

gboolean motion_notify_event_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data);
gboolean button_press_event_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean upload_button_clicked_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_window_destroy();
void rotateHandler(GdkEventMotion *event, double *theta, double *phi);
void clear_surface();

struct obj *testObj;
cairo_surface_t *surface;
int canvas_height, canvas_width;

void init_gui(int argc, char **argv) {
	testObj = (struct obj*)malloc(sizeof(struct obj));
    read_object("african_head.obj", testObj);

    GtkBuilder *builder;
    GtkWidget *window;

    gtk_init(&argc, &argv);
 
    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "render.glade", NULL);
 
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    gtk_builder_connect_signals(builder, NULL);
 
    g_object_unref(builder);
 
    gtk_widget_show(window);                
    gtk_main();
}

gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
    if (surface)
        cairo_surface_destroy (surface);
    canvas_width = gtk_widget_get_allocated_width(widget);
    canvas_height = gtk_widget_get_allocated_height(widget);
    surface = gdk_window_create_similar_surface (gtk_widget_get_window(widget), CAIRO_CONTENT_COLOR, canvas_width, canvas_height);
    clear_surface();
    gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    init_buffer(canvas_width, canvas_height);
    return TRUE;
}

gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data) {
    cairo_set_source_surface (cr, surface, 0, 0);
    cairo_paint (cr);
    return FALSE;
}

gboolean button_press_event_cb (GtkWidget *widget, GdkEventButton *event, gpointer data) {
    return TRUE;
}

gboolean upload_button_clicked_cb (GtkWidget *widget, GdkEventButton *event, gpointer data) {
    return TRUE;  
}

gboolean motion_notify_event_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    static double phi = 0, theta = 0;
    if (surface == NULL)
        return FALSE;
    if (event->state & GDK_BUTTON1_MASK) {
        rotateHandler(event, &phi, &theta);
        cairo_t *cr = cairo_create(surface);
        gdk_pixbuf_fill(buff, 0x000000);
        draw_obj(testObj, Red, theta, phi);
        gdk_cairo_set_source_pixbuf(cr, buff, 5, 5);
        cairo_paint(cr);
        cairo_fill (cr);
        cairo_destroy(cr);
        gtk_widget_queue_draw (widget);
    }
    return TRUE;
}

void on_window_destroy() {
    if (surface)
      cairo_surface_destroy (surface);
    gtk_main_quit ();
}

void rotateHandler(GdkEventMotion *event, double *theta, double *phi) {
    static int prev_y = 0, prev_x = 0;
    static int ycountpos = 0, xcountpos = 0, xcountneg = 0, ycountneg = 0; 
    int x = event->x, y = event->y;
    int dx = x - prev_x, dy = dy - y;
    prev_x = x; prev_y = y;
    if (dy > 0) ycountpos++; if (dy < 0) ycountneg++;
    if (dx > 0) xcountpos++; if (dx < 0) xcountneg++;
    if (ycountpos == 1) {
        *phi += 1;
        ycountpos = 0;
    }
    if (ycountneg == 1) {
        *phi -= 1;
        ycountneg = 0;
    }
    if (xcountpos == 1) {
        *theta +=1;
        xcountpos = 0;
    }
    if (xcountneg == 1) {
        *theta -= 1;
        xcountneg = 0;
    }
}

void clear_surface(void) {
    cairo_t *cr;
    cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_destroy(cr);
}