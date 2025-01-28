#include <gtk/gtk.h>
#include <math.h>

#include "gtk_gui.h"
#include "renderer.h"

#define M_PI 3.14159265358979323846

#define SQR(x) ((x) * (x))
#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)
#define RAD_TO_DEG(rad) ((rad) * 180.0 / M_PI)

gboolean motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data);
gboolean button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean on_zoom_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean on_rotate_y_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean on_rotate_z_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean on_rotate_x_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean on_change_rho_slider_value_changed(GtkRange *range, gpointer user_data);
gboolean scroll_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data);
void on_shading_combo_changed(GtkComboBoxText *combo, gpointer user_data);
void update_rotation_angles();

void on_toggle_button_clicked(GtkButton *button, gpointer user_data);
gboolean upload_button_clicked_cb(GtkWidget *widget, gpointer data);

void on_window_destroy();
void draw_object_on_canvas(GtkWidget *widget);
gboolean is_object_loaded();
gboolean timer_exe(GtkWidget *window);
GtkRevealer *global_revealer;
GtkWidget *global_canvas;
GtkRange *global_rho_slider;

struct obj *object;
cairo_surface_t *surface;
cairo_t *cr;

int canvas_height, canvas_width;
double x, y, z = 0;
double current_x_alpha = 0;
double current_y_beta = 0;
double current_z_gamma = 0;
double current_scale_value = 50;

static gboolean is_left_mouse_pressed = FALSE;
static gboolean is_right_mouse_pressed = FALSE;
static int prev_y = 0, prev_x = 0;

static int currently_drawing = 0;

struct color Red = {255, 0, 0};
struct color Black = {200, 200, 200};
struct color White = {255, 255, 255};

int WHITE_BACKGROUND = 0xffffffff;
int BLACK_BACKGROUND = 0x00000000;

void init_gui(struct arguments arguments, int argc, char **argv)
{
    int fps_interval = (arguments.fps == 30) ? 33 : 16;

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
    GObject *rho_slider = gtk_builder_get_object(builder, "change_rho_slider");

    global_canvas = GTK_WIDGET(canvas);
    global_revealer = GTK_REVEALER(revealer);
    global_rho_slider = GTK_RANGE(rho_slider);

    g_object_unref(builder);
    gtk_widget_show(window);

    if (arguments.file)
    {
        read_object(arguments.file, object);
        gtk_revealer_set_reveal_child(global_revealer, TRUE);
        set_object_to_render(object);
        int translation_value = get_translation();
        gtk_range_set_value(global_rho_slider, translation_value);
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
    cr = cairo_create(surface);
    gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK);
    initialize_canvas_buffer(canvas_width, canvas_height);
    initialize_renderer(canvas_width, canvas_height);
    set_color(White, Black);
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
        g_free(filename);
        gtk_revealer_set_reveal_child(global_revealer, TRUE);
        set_object_to_render(object);
        int translation_value = get_translation();
        gtk_range_set_value(global_rho_slider, translation_value);
    }
    gtk_widget_destroy(dialog);
    return TRUE;
}

gboolean on_zoom_slider_value_changed(GtkRange *range, gpointer user_data)
{
    int zoom_value = (int)gtk_range_get_value(range);
    double zoom = (double)(zoom_value) / 100 - 0.5;
    set_scale(1 + (zoom * 2));
    return TRUE;
}

gboolean on_rotate_y_slider_value_changed(GtkRange *range, gpointer user_data)
{
    current_y_beta = (int)gtk_range_get_value(range);
    update_rotation_angles();
    return TRUE;
}

gboolean on_rotate_z_slider_value_changed(GtkRange *range, gpointer user_data)
{
    current_z_gamma = (int)gtk_range_get_value(range);
    update_rotation_angles();
    return TRUE;
}

gboolean on_rotate_x_slider_value_changed(GtkRange *range, gpointer user_data)
{
    current_x_alpha = (int)gtk_range_get_value(range);
    update_rotation_angles();
    return TRUE;
}

gboolean on_change_rho_slider_value_changed(GtkRange *range, gpointer user_data)
{
    int rho = (int)gtk_range_get_value(range);
    set_translation(rho);
    return TRUE;
}

void update_rotation_angles()
{
    set_cartesian_translation(DEG_TO_RAD(current_x_alpha), DEG_TO_RAD(current_y_beta), DEG_TO_RAD(current_z_gamma));
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
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
    }
    gtk_main_quit();
}

void draw_object_on_canvas(GtkWidget *widget)
{
    fill_buffer(BLACK_BACKGROUND);

    if (is_object_loaded())
    {
        render();
    }

    set_pixbuf_for_surface(cr);
    cairo_paint(cr);
    gtk_widget_queue_draw(widget);
}

gboolean is_object_loaded()
{
    return object->number_of_triangles > 0;
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

gboolean motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    if (event->state)
    {
        int x = event->x, y = event->y;
        int dx = x - prev_x, dy = y - prev_y;
        prev_x = x;
        prev_y = y;

        if (is_left_mouse_pressed)
        {
            current_x_alpha -= dy / 4;
            current_y_beta += dx / 4;
            update_rotation_angles();
        }
        else if (is_right_mouse_pressed)
        {
            move_obj_relative(dx, dy);
        }
    }
    return TRUE;
}

// Callback for button-press-event
gboolean button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    if (event->button == GDK_BUTTON_PRIMARY)
    { // Check if the left mouse button is pressed
        is_left_mouse_pressed = TRUE;
        prev_x = event->x;
        prev_y = event->y;
    }
    else if (event->button == GDK_BUTTON_SECONDARY)
    {
        is_right_mouse_pressed = TRUE;
    }
    return TRUE; // Returning TRUE stops the event from propagating further
}

// Callback for button-release-event
gboolean button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    if (event->button == GDK_BUTTON_PRIMARY)
    {
        is_left_mouse_pressed = FALSE;
    }
    else if (event->button == GDK_BUTTON_SECONDARY)
    {
        is_right_mouse_pressed = FALSE;
    }
    return TRUE;
}

gboolean scroll_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
    switch (event->direction)
    {
    case GDK_SCROLL_UP:
        current_scale_value += 1;
        break;
    case GDK_SCROLL_DOWN:
        current_scale_value -= 1;
        break;
    case GDK_SCROLL_SMOOTH:
        if (event->delta_y > 0)
            current_scale_value += 1;
        else
            current_scale_value -= 1;
        break;
    }

    current_scale_value = MAX(10, current_scale_value);
    current_scale_value = MIN(200, current_scale_value);

    double zoom = (double)(current_scale_value) / 100 - 0.5;
    set_scale(1 + (zoom * 2));

    return TRUE;
}

void on_shading_combo_changed(GtkComboBoxText *combo, gpointer user_data)
{
    const gchar *selected_option = gtk_combo_box_text_get_active_text(combo);
    if (selected_option)
    {
        g_print("Selected shading option: %s\n", selected_option);

        // Perform actions based on the selected option
        if (g_strcmp0(selected_option, "No shading") == 0)
        {
            set_shading_type(NO_SHADING);
        }
        else if (g_strcmp0(selected_option, "Flat") == 0)
        {
            set_shading_type(FLAT);
        }
        else if (g_strcmp0(selected_option, "Gouraud") == 0)
        {
            set_shading_type(GOURAUD);
        }

        g_free((gchar *)selected_option); // Free the string
    }
}

void on_toggle_draw_triangles_toggled(GtkToggleButton *toggle_button, gpointer user_data)
{
    gboolean is_active = gtk_toggle_button_get_active(toggle_button);
    set_draw_triangles(is_active);
}

void on_toggle_draw_vertices_toggled(GtkToggleButton *toggle_button, gpointer user_data)
{
    gboolean is_active = gtk_toggle_button_get_active(toggle_button);
    set_draw_vertices(is_active);
}

void on_toggle_draw_edges_toggled(GtkToggleButton *toggle_button, gpointer user_data)
{
    gboolean is_active = gtk_toggle_button_get_active(toggle_button);
    set_draw_edges(is_active);
}