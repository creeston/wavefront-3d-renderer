#include "gtk_gui.h"

#include <gtk/gtk.h>
#include "renderer.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)
#define RAD_TO_DEG(rad) ((rad) * 180.0 / M_PI)

gboolean motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data);
gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean scroll_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data);
gboolean on_zoom_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean on_rotate_phi_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean on_rotate_theta_slider_value_changed(GtkRange *range, gpointer user_data);
void on_toggle_button_clicked(GtkButton *button, gpointer user_data);
gboolean upload_button_clicked_cb(GtkWidget *widget, gpointer data);

void on_window_destroy();
void rotateHandler(GdkEventMotion *event, double *theta, double *phi);
void clear_surface();
void draw_object_on_canvas(GtkWidget *widget);

GtkRevealer *global_revealer;
GtkWidget *global_canvas;

struct obj *objects[1];
int objects_amount;
cairo_surface_t *surface;
int canvas_height, canvas_width;
double phi = 0, theta = 0, rho = 0;
double x, y, z = 0;

void cartesian_to_spherical(double x, double y, double z, double *rho, double *theta, double *phi);
void angles_to_spherical(double x_deg, double y_deg, double z_deg, double *theta_deg, double *phi_deg);

void init_gui(int argc, char **argv)
{
    objects[0] = (struct obj *)malloc(sizeof(struct obj));

    GtkBuilder *builder;
    GtkWidget *window;
    gtk_init(&argc, &argv);
    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "render.glade", NULL);
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    gtk_builder_connect_signals(builder, NULL);

    GObject *revealer = gtk_builder_get_object(builder, "control_revealer");
    GObject *canvas = gtk_builder_get_object(builder, "render_area");

    global_canvas = GTK_WIDGET(canvas);
    global_revealer = GTK_REVEALER(revealer);

    g_object_unref(builder);
    gtk_widget_show(window);
    gtk_main();
}

gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
    if (surface)
    {
        cairo_surface_destroy(surface);
    }

    canvas_width = gtk_widget_get_allocated_width(widget);
    canvas_height = gtk_widget_get_allocated_height(widget);
    surface = gdk_window_create_similar_surface(gtk_widget_get_window(widget), CAIRO_CONTENT_COLOR, canvas_width, canvas_height);
    clear_surface();
    gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
    init_buffer(canvas_width, canvas_height);
    return TRUE;
}

gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    return FALSE;
}

gboolean upload_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;
    dialog = gtk_file_chooser_dialog_new("Open File", NULL, action, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        read_object(filename, objects[0]);
        objects_amount = 1;
        g_free(filename);

        GdkEventMotion event;
        event.x = 0;
        event.y = 0;
        rotateHandler(&event, &phi, &theta); // has to be called to initialize the object properly
        rho = 3000.0;

        draw_object_on_canvas(global_canvas);
        gtk_revealer_set_reveal_child(global_revealer, TRUE);
    }
    gtk_widget_destroy(dialog);

    return TRUE;
}

gboolean motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    if (surface == NULL)
        return FALSE;
    if (event->state & GDK_BUTTON1_MASK)
    {
        rotateHandler(event, &phi, &theta);
        draw_object_on_canvas(widget);
    }
    return TRUE;
}

gboolean scroll_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
    switch (event->direction)
    {
    case GDK_SCROLL_UP:
        d -= 20;
        break;
    case GDK_SCROLL_DOWN:
        d += 20;
        break;
    case GDK_SCROLL_LEFT:
        c1 -= 0.1;
        break;
    case GDK_SCROLL_RIGHT:
        c1 += 0.1;
        break;
    case GDK_SCROLL_SMOOTH:
        if (event->delta_y > 0)
            d -= 20;
        else
            d += 20;
        break;
    }

    draw_object_on_canvas(widget);
    return TRUE;
}

gboolean on_zoom_slider_value_changed(GtkRange *range, gpointer user_data)
{
    int zoom_value = (int)gtk_range_get_value(range);
    double zoom = (double)(zoom_value) / 100 - 0.5;
    d = initial_d + initial_d * (zoom * 2);
    GtkWidget *canvas = GTK_WIDGET(user_data);
    draw_object_on_canvas(canvas);
    return TRUE;
}

gboolean on_rotate_phi_slider_value_changed(GtkRange *range, gpointer user_data)
{
    int rotate_phi = (int)gtk_range_get_value(range);
    phi = rotate_phi;
    draw_object_on_canvas(global_canvas);
    return TRUE;
}

gboolean on_rotate_theta_slider_value_changed(GtkRange *range, gpointer user_data)
{
    int rotate_theta = (int)gtk_range_get_value(range);
    theta = rotate_theta;
    draw_object_on_canvas(global_canvas);
    return TRUE;
}

void on_toggle_button_clicked(GtkButton *button, gpointer user_data)
{
    if (objects_amount == 0)
    {
        return;
    }
    GtkRevealer *revealer = GTK_REVEALER(user_data);
    gboolean is_revealed = gtk_revealer_get_reveal_child(revealer);
    gtk_revealer_set_reveal_child(revealer, !is_revealed);
}

void on_window_destroy()
{
    if (surface)
        cairo_surface_destroy(surface);
    gtk_main_quit();
}

void rotateHandler(GdkEventMotion *event, double *theta, double *phi)
{
    static int prev_y = 0, prev_x = 0;
    static int ycountpos = 0, xcountpos = 0, xcountneg = 0, ycountneg = 0;
    int x = event->x, y = event->y;
    int dx = x - prev_x, dy = dy - y;
    prev_x = x;
    prev_y = y;
    if (dy > 0)
        ycountpos++;
    if (dy < 0)
        ycountneg++;
    if (dx > 0)
        xcountpos++;
    if (dx < 0)
        xcountneg++;
    if (ycountpos == 1)
    {
        *phi += 1;
        ycountpos = 0;
    }
    if (ycountneg == 1)
    {
        *phi -= 1;
        ycountneg = 0;
    }
    if (xcountpos == 1)
    {
        *theta += 1;
        xcountpos = 0;
    }
    if (xcountneg == 1)
    {
        *theta -= 1;
        xcountneg = 0;
    }
}

void draw_object_on_canvas(GtkWidget *widget)
{
    cairo_t *cr = cairo_create(surface);
    gdk_pixbuf_fill(buff, Background);

    draw_objects(objects, objects_amount, Black, theta, phi, rho);
    gdk_cairo_set_source_pixbuf(cr, buff, 5, 5);
    cairo_paint(cr);

    // cairo_fill(cr); isn't necessary?

    cairo_destroy(cr);
    gtk_widget_queue_draw(widget);
}

void clear_surface(void)
{
    cairo_t *cr;
    cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_destroy(cr);
}

void cartesian_to_spherical(double x_deg, double y_deg, double z_deg, double *rho, double *theta, double *phi)
{
    // Convert input angles from degrees to radians
    double x = cos(DEG_TO_RAD(x_deg));
    double y = cos(DEG_TO_RAD(y_deg));
    double z = cos(DEG_TO_RAD(z_deg));

    // Calculate rho (magnitude of the vector)
    *rho = sqrt(x * x + y * y + z * z) * 1000;

    // Calculate theta (azimuthal angle) in radians and convert to degrees
    *theta = RAD_TO_DEG(atan2(y, x));

    // Calculate phi (polar angle) in radians and convert to degrees
    *phi = RAD_TO_DEG(acos(z / *rho));
}

void angles_to_spherical(double x_deg, double y_deg, double z_deg, double *theta_deg, double *phi_deg)
{
    // Convert angles from degrees to radians
    double x_rad = DEG_TO_RAD(x_deg);
    double y_rad = DEG_TO_RAD(y_deg);
    double z_rad = DEG_TO_RAD(z_deg);

    // Calculate unit vector components
    double u_x = cos(y_rad) * cos(z_rad);
    double u_y = sin(x_rad) * cos(z_rad);
    double u_z = sin(z_rad);

    // Calculate spherical angles
    rho = 3000;
    *theta_deg = RAD_TO_DEG(atan2(u_y, u_x)); // Azimuthal angle (XY-plane)
    *phi_deg = RAD_TO_DEG(acos(u_z));         // Polar angle (from Z-axis)
}