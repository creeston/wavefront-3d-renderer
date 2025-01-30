#include <gtk/gtk.h>
#include <math.h>

#include "gtk_gui.h"
#include "renderer.h"
#include "utils.h"
#include "wavefront_object_reader.h"
#include "gtk_drawing.h"

const struct color GREY = {200, 200, 200};
const struct color WHITE = {255, 255, 255};

const int BLACK_BACKGROUND = 0x00000000;
const int MAX_NUMBER_OF_TRIANGLES = 100000;
const int RENDER_INTERVAL_MS = 16;
const int DEFAULT_SCALE_VALUE = 50;
const int MIN_SCALE_VALUE = 25;
const int MAX_SCALE_VALUE = 150;
const int SCALE_STEP = 5;
const float MAX_CAMERA_DISTANCE_MULTIPLIER = 1.5;
const float MIN_CAMERA_DISTANCE_MULTIPLIER = 0.5;

gboolean motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data);
gboolean mouse_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean mouse_button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean drawing_area_configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean on_change_camera_distance_slider_value_cb(GtkRange *range, gpointer user_data);
gboolean scroll_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data);
gboolean upload_button_clicked_cb(GtkWidget *widget, gpointer data);
gboolean on_shading_select_changed_cb(GtkComboBoxText *combo, gpointer user_data);
gboolean render_timer(GtkWidget *window);

GtkRevealer *global_revealer;
GtkWidget *global_canvas;
GtkRange *global_camera_distance_slider;
GtkLabel *global_fps_label;
GtkAdjustment *global_camera_distance_adjustment;
GtkWindow *global_window;
cairo_surface_t *cairo_surface;
cairo_t *cairo_context;

void update_rotation_angles();
void reset_rotation_angles();
void destroy_object(struct wavefront_obj object);
void on_window_destroy();
int update_active_object(struct wavefront_obj new_object);

#define DISPLAY_ERROR_DIALOG(message)                           \
    do                                                          \
    {                                                           \
        GtkWidget *dialog = gtk_message_dialog_new(             \
            global_window, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, \
            GTK_BUTTONS_OK, message);                           \
        gtk_dialog_run(GTK_DIALOG(dialog));                     \
        gtk_widget_destroy(dialog);                             \
    } while (0)

struct wavefront_obj active_object;

double current_x_alpha = 0;
double current_y_beta = 0;
double current_z_gamma = 0;
double current_scale_value = DEFAULT_SCALE_VALUE;

gboolean is_left_mouse_pressed = FALSE;
gboolean is_right_mouse_pressed = FALSE;
int previous_mouse_position_y = 0, previous_mouse_position_x = 0;

int is_currently_rendering = 0;
int render_count = 0;
clock_t start_fps_measurement;

void init_gui(struct arguments arguments, int argc, char **argv)
{
    GtkBuilder *builder;
    GtkWidget *window;
    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_resource(builder, "/com/creeston/wavefront-3d-renderer/gui.glade", NULL);
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    gtk_builder_connect_signals(builder, NULL);

    GObject *revealer = gtk_builder_get_object(builder, "control_revealer");
    GObject *canvas = gtk_builder_get_object(builder, "render_area");
    GObject *camera_distance_slider = gtk_builder_get_object(builder, "change_camera_distance_slider");
    GObject *fps_label = gtk_builder_get_object(builder, "fps_label");
    GObject *camera_distance_adjustment = gtk_builder_get_object(builder, "camera_distance_adjustment");

    global_canvas = GTK_WIDGET(canvas);
    global_revealer = GTK_REVEALER(revealer);
    global_camera_distance_slider = GTK_RANGE(camera_distance_slider);
    global_fps_label = GTK_LABEL(fps_label);
    global_camera_distance_adjustment = GTK_ADJUSTMENT(camera_distance_adjustment);
    global_window = GTK_WINDOW(window);

    g_object_unref(builder);
    gtk_widget_show(window);

    if (arguments.file) // If a file is provided, load the object
    {
        struct wavefront_obj new_object;
        int read_result = read_object(arguments.file, &new_object);
        if (read_result)
        {
            update_active_object(new_object);
        }
        else
        {
            DISPLAY_ERROR_DIALOG("The file could not be read.");
        }
    }

    (void)g_timeout_add(RENDER_INTERVAL_MS, (GSourceFunc)render_timer, global_canvas);
    gtk_main();
}

gboolean drawing_area_configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
    if (cairo_surface)
    {
        cairo_surface_destroy(cairo_surface);
    }

    int canvas_width = gtk_widget_get_allocated_width(widget);
    int canvas_height = gtk_widget_get_allocated_height(widget);
    cairo_surface = gdk_window_create_similar_surface(gtk_widget_get_window(widget), CAIRO_CONTENT_COLOR, canvas_width, canvas_height);
    cairo_context = cairo_create(cairo_surface);
    gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK);
    initialize_canvas_buffer(canvas_width, canvas_height);
    initialize_renderer(canvas_width, canvas_height);
    set_colors(BLACK_BACKGROUND, WHITE, GREY);
    current_scale_value = DEFAULT_SCALE_VALUE;
    return TRUE;
}

gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    set_pixbuf_for_surface(cr);
    cairo_paint(cr);
    render_count++;
    g_atomic_int_set(&is_currently_rendering, 0);

    if (render_count >= 50 && render_count != 0)
    {
        clock_t end_fps_measurement = clock();
        float total_render_time = (float)(end_fps_measurement - start_fps_measurement) / CLOCKS_PER_SEC;
        float fps = render_count / total_render_time;
        render_count = 0;
        start_fps_measurement = clock();
        gtk_label_set_label(global_fps_label, g_strdup_printf("FPS: %.2f", fps));
    }
    return FALSE;
}

gboolean upload_button_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open File",
        global_window,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel",
        GTK_RESPONSE_CANCEL,
        "_Open",
        GTK_RESPONSE_ACCEPT,
        NULL);

    int is_upload_dialog_open = TRUE;
    while (is_upload_dialog_open)
    {
        gint res = gtk_dialog_run(GTK_DIALOG(dialog));
        if (res == GTK_RESPONSE_CANCEL)
        {
            gtk_widget_destroy(dialog);
            is_upload_dialog_open = FALSE;
            return TRUE;
        }
        else if (res == GTK_RESPONSE_ACCEPT)
        {
            char *filename;
            GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
            filename = gtk_file_chooser_get_filename(chooser);
            struct wavefront_obj new_object;
            int read_result = read_object(filename, &new_object);
            g_free(filename);

            if (!read_result)
            {
                DISPLAY_ERROR_DIALOG("The file could not be read.");
                continue;
            }

            int update_result = update_active_object(new_object);
            if (update_result)
            {
                gtk_widget_destroy(dialog);
                is_upload_dialog_open = FALSE;
                return TRUE;
            }
        }
    }

    return TRUE;
}

gboolean on_change_camera_distance_slider_value_cb(GtkRange *range, gpointer user_data)
{
    int value = (int)gtk_range_get_value(range);
    set_camera_distance(value);
    return TRUE;
}

void update_rotation_angles()
{
    set_cartesian_translation(DEG_TO_RAD(current_x_alpha), DEG_TO_RAD(current_y_beta), DEG_TO_RAD(current_z_gamma));
}

void on_window_destroy()
{
    if (cairo_surface)
    {
        cairo_destroy(cairo_context);
        cairo_surface_destroy(cairo_surface);
    }
    destroy_object(active_object);
    gtk_main_quit();
}

gboolean render_timer(GtkWidget *drawing_area)
{
    if (active_object.number_of_triangles == 0)
    {
        return TRUE;
    }

    int drawing_status = g_atomic_int_get(&is_currently_rendering);
    if (drawing_status == 0)
    {
        g_atomic_int_set(&is_currently_rendering, 1);
        render();
        gtk_widget_queue_draw(drawing_area);
    }

    return TRUE;
}

gboolean motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    if (event->state)
    {
        int x = event->x, y = event->y;
        int dx = x - previous_mouse_position_x, dy = y - previous_mouse_position_y;
        previous_mouse_position_x = x;
        previous_mouse_position_y = y;

        if (is_left_mouse_pressed)
        {
            current_x_alpha -= (double)dy / 4;
            current_y_beta += (double)dx / 4;
            update_rotation_angles();
        }
        else if (is_right_mouse_pressed)
        {
            current_z_gamma += (double)dx / 4;
            update_rotation_angles();
        }
    }
    return TRUE;
}

gboolean mouse_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    previous_mouse_position_x = event->x;
    previous_mouse_position_y = event->y;

    if (event->button == GDK_BUTTON_PRIMARY)
    {
        is_left_mouse_pressed = TRUE;
        gdk_window_set_cursor(gtk_widget_get_window(widget), gdk_cursor_new_from_name(gdk_display_get_default(), "grabbing"));
    }
    else if (event->button == GDK_BUTTON_SECONDARY)
    {
        is_right_mouse_pressed = TRUE;
        gdk_window_set_cursor(gtk_widget_get_window(widget), gdk_cursor_new_from_name(gdk_display_get_default(), "grabbing"));
    }
    return TRUE;
}

gboolean mouse_button_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    gdk_window_set_cursor(gtk_widget_get_window(widget), gdk_cursor_new_from_name(gdk_display_get_default(), "default"));
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
        current_scale_value += SCALE_STEP;
        break;
    case GDK_SCROLL_DOWN:
        current_scale_value -= SCALE_STEP;
        break;
    }

    current_scale_value = MAX(MIN_SCALE_VALUE, current_scale_value);
    current_scale_value = MIN(MAX_SCALE_VALUE, current_scale_value);

    double zoom = (double)(current_scale_value) / 100 - 0.5;
    set_scale(1 + (zoom * 2));

    return TRUE;
}

gboolean on_shading_select_changed_cb(GtkComboBoxText *combo, gpointer user_data)
{
    const gchar *selected_option = gtk_combo_box_text_get_active_text(combo);
    if (selected_option)
    {
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

        g_free((gchar *)selected_option);
    }

    return TRUE;
}

void reset_rotation_angles()
{
    current_x_alpha = 0;
    current_y_beta = 0;
    current_z_gamma = 0;
    current_scale_value = DEFAULT_SCALE_VALUE;
    update_rotation_angles();
}

int update_active_object(struct wavefront_obj new_object)
{
    if (new_object.number_of_triangles > MAX_NUMBER_OF_TRIANGLES)
    {
        DISPLAY_ERROR_DIALOG("The object has too many triangles to render.");
        destroy_object(new_object);
        return FALSE;
    }

    destroy_object(active_object);
    active_object = new_object;

    reset_rotation_angles();
    set_object_to_render(active_object);

    int camera_distance_value = get_camera_distance();
    gtk_adjustment_set_upper(global_camera_distance_adjustment, (int)camera_distance_value * MAX_CAMERA_DISTANCE_MULTIPLIER);
    gtk_adjustment_set_lower(global_camera_distance_adjustment, (int)camera_distance_value * MIN_CAMERA_DISTANCE_MULTIPLIER);
    gtk_range_set_value(global_camera_distance_slider, camera_distance_value);

    gtk_revealer_set_reveal_child(global_revealer, TRUE);

    render_count = 0;
    start_fps_measurement = clock();

    return TRUE;
}

void destroy_object(struct wavefront_obj object)
{
    if (object.vertices != NULL)
    {
        free(object.vertices);
    }

    if (object.triangles != NULL)
    {
        free(object.triangles);
    }

    object.vertices = NULL;
    object.triangles = NULL;
    object.number_of_triangles = 0;
    object.number_of_vertices = 0;
}
