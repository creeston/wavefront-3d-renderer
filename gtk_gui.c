#include <gtk/gtk.h>
#include <math.h>

#include "gtk_gui.h"
#include "renderer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)
#define RAD_TO_DEG(rad) ((rad) * 180.0 / M_PI)

gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean on_zoom_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean on_rotate_phi_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean on_rotate_theta_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean on_rotate_x_slider_value_changed(GtkRange *range, gpointer user_data);
void on_toggle_button_clicked(GtkButton *button, gpointer user_data);
gboolean upload_button_clicked_cb(GtkWidget *widget, gpointer data);

void on_window_destroy();
void clear_surface();
void draw_object_on_canvas(GtkWidget *widget);
gboolean is_object_loaded();
gboolean timer_exe(GtkWidget *window);
GtkRevealer *global_revealer;
GtkWidget *global_canvas;

struct obj *object;
cairo_surface_t *surface;
int canvas_height, canvas_width;
double phi = 0, theta = 0, rho = 3000;
double x, y, z = 0;
double current_x_alpha = 0;
double current_y_beta = 0;
double current_z_gamma = 0;

void rotate_around_x(int alpha, double *rho, double *theta, double *phi);
void rotate_around_y(int alpha, double *rho, double *theta, double *phi);
void rotate_around_z(int alpha, double *rho, double *theta, double *phi);

static int currently_drawing = 0;

struct color Red = {255, 0, 0};
struct color Black = {100, 100, 100};
int Background = 0xffffffff;

void init_gui(struct arguments arguments, int argc, char **argv)
{
    int fps_interval = (arguments.fps == 30) ? 33 : 16;
    fps_interval = 1000;

    GtkBuilder *builder;
    GtkWidget *window;
    gtk_init(&argc, &argv);
    object = (struct obj *)malloc(sizeof(struct obj));
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

    if (arguments.file)
    {
        read_object(arguments.file, object);
        rho = 3000.0;
        gtk_revealer_set_reveal_child(global_revealer, TRUE);
    }

    (void)g_timeout_add(fps_interval, (GSourceFunc)timer_exe, window);
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
    initialize_canvas_buffer(canvas_width, canvas_height);
    initialize_renderer(canvas_width, canvas_height);
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
        read_object(filename, object);
        rho = 3000.0;
        g_free(filename);
        gtk_revealer_set_reveal_child(global_revealer, TRUE);
    }
    gtk_widget_destroy(dialog);
    return TRUE;
}

gboolean on_zoom_slider_value_changed(GtkRange *range, gpointer user_data)
{
    int zoom_value = (int)gtk_range_get_value(range);
    double zoom = (double)(zoom_value) / 100 - 0.5;
    change_scale(1 + (zoom * 2));
    return TRUE;
}

gboolean on_rotate_phi_slider_value_changed(GtkRange *range, gpointer user_data)
{
    int rotate_beta = (int)gtk_range_get_value(range);
    int beta = rotate_beta - current_y_beta;
    rotate_around_y(beta, &rho, &theta, &phi);
    current_y_beta = rotate_beta;
    return TRUE;
}

gboolean on_rotate_theta_slider_value_changed(GtkRange *range, gpointer user_data)
{
    int rotate_gamma = (int)gtk_range_get_value(range);
    int gamma = rotate_gamma - current_z_gamma;
    rotate_around_z(gamma, &rho, &theta, &phi);
    current_z_gamma = rotate_gamma;
    return TRUE;
}

gboolean on_rotate_x_slider_value_changed(GtkRange *range, gpointer user_data)
{
    int rotate_alpha = (int)gtk_range_get_value(range);
    int alpha = rotate_alpha - current_x_alpha;
    rotate_around_x(alpha, &rho, &theta, &phi);
    current_x_alpha = rotate_alpha;
    return TRUE;
}

void on_toggle_button_clicked(GtkButton *button, gpointer user_data)
{
    if (!is_object_loaded())
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
    {
        cairo_surface_destroy(surface);
    }
    gtk_main_quit();
}

void draw_object_on_canvas(GtkWidget *widget)
{
    fill_buffer(Background);
    draw_obj(object, Black, theta, phi, rho);

    cairo_t *cr = cairo_create(surface);
    set_pixbuf_for_surface(cr);
    cairo_paint(cr);
    cairo_fill(cr);
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

float sperical_to_cartesian_x()
{
    return rho * sin(DEG_TO_RAD(theta)) * cos(DEG_TO_RAD(phi));
}

float sperical_to_cartesian_y()
{
    return rho * sin(DEG_TO_RAD(theta)) * sin(DEG_TO_RAD(phi));
}

float sperical_to_cartesian_z()
{
    return rho * cos(DEG_TO_RAD(theta));
}

float cartesian_to_sperical_theta(int z)
{
    return RAD_TO_DEG(acos(z / rho));
}

float cartesian_to_sperical_phi(int x, int y)
{
    return RAD_TO_DEG(atan2(y, x));
}

void rotate_around_x(int alpha, double *rho, double *theta, double *phi)
{
    double x = sperical_to_cartesian_x();
    double y = sperical_to_cartesian_y();
    double z = sperical_to_cartesian_z();

    double newX = x;
    double newY = y * cos(DEG_TO_RAD(alpha)) - z * sin(DEG_TO_RAD(alpha));
    double newZ = y * sin(DEG_TO_RAD(alpha)) + z * cos(DEG_TO_RAD(alpha));

    *theta = cartesian_to_sperical_theta(newZ);
    *phi = cartesian_to_sperical_phi(newX, newY);
}

void rotate_around_y(int alpha, double *rho, double *theta, double *phi)
{
    double x = sperical_to_cartesian_x();
    double y = sperical_to_cartesian_y();
    double z = sperical_to_cartesian_z();

    double newX = x * cos(DEG_TO_RAD(alpha)) + z * sin(DEG_TO_RAD(alpha));
    double newY = y;
    double newZ = -x * sin(DEG_TO_RAD(alpha)) + z * cos(DEG_TO_RAD(alpha));

    *theta = cartesian_to_sperical_theta(newZ);
    *phi = cartesian_to_sperical_phi(newX, newY);
}

void rotate_around_z(int alpha, double *rho, double *theta, double *phi)
{
    double x = sperical_to_cartesian_x();
    double y = sperical_to_cartesian_y();
    double z = sperical_to_cartesian_z();

    double newX = x * cos(DEG_TO_RAD(alpha)) - y * sin(DEG_TO_RAD(alpha));
    double newY = x * sin(DEG_TO_RAD(alpha)) + y * cos(DEG_TO_RAD(alpha));
    double newZ = z;

    *theta = cartesian_to_sperical_theta(newZ);
    *phi = cartesian_to_sperical_phi(newX, newY);
}

gboolean is_object_loaded()
{
    return object->ntr > 0;
}

gboolean timer_exe(GtkWidget *window)
{
    static gboolean first_execution = TRUE;
    if (!is_object_loaded())
    {
        return TRUE;
    }

    int drawing_status = g_atomic_int_get(&currently_drawing);
    if (drawing_status == 0)
    {
        g_atomic_int_set(&currently_drawing, 1);
        draw_object_on_canvas(global_canvas);
        g_atomic_int_set(&currently_drawing, 0);
    }

    return TRUE;
}