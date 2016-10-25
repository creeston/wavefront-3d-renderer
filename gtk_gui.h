#include "renderer.h"

void init_gui(int argc, char **argv);
gboolean motion_notify_event_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data);
gboolean button_press_event_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean upload_button_clicked_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_window_destroy();
void rotateHandler(GdkEventMotion *event, double *theta, double *phi);

struct obj *testObj;