#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* Surface to store current scribbles */
#define BIG 1e30
#define Xvp_min 5
#define Xvp_max 605
#define Yvp_min 5
#define Yvp_max 605
#define sqr(x) ((x)*(x))
#define CROP 1
#define NOCROP 2


// Likely the problem is in z-buffer
// + very slow drawing, reconsided pix drawing procedure

struct ver {
    double x,y,z;
    int *connect;
    double a, b ,c;
};
struct tr{
    int A,B,C;
    double a,b,c,h;
    int first;
};
struct obj {
    struct ver *fvertex, *vertex;
    int nvr;
    struct tr* triangle;
    int ntr;
    int n;
};
struct color{
    int r,g,b;
};
struct v2D{
    int x,y;
    float depth;
};
struct v3D {
    double x,y,z;
};

struct color Red = {255, 0 ,0};
struct v3D lightdir = {0,0,1};

void rast_line(struct v2D A, struct v2D B, struct v2D C, struct color Cl);
void line(int x0, int y0, int x1, int y1, struct color Cl, int flag);
void drawpix(int x, int y, struct color clr);
void read_object(char *filename, struct obj *object);
void draw_obj(GdkPixbuf* buffer, struct obj *object, struct color C, double theta, double phi);
void DrawTr(struct tr* triangle, int ntr, struct ver* vertex, struct color Cl);
void extra_init();
void CalcVertex(struct ver* fvertex, struct ver** vertex, int nvr, double *Range);
void extraCalcVertex(struct ver** vertex, int nvr, struct tr* triangle);
void scale_calc(double Xmin, double Xmax, double Ymin, double Ymax);
void CalcTr(struct ver* vertex, struct tr** triangle, int ntr, int nvr);
void Rast_Tr(struct v2D A, struct v2D B, struct v2D C, struct color Cl);
void ReadVertex(char *str, struct ver **fvertex, int *nvr);
void ReadFace(char *str, struct tr** triangle, int *ntr);
void SPolyDiv(int *poly, int npoly, struct tr* triangle, int *ntr);
void RememberVertex(struct tr* triangle, int ntr, struct ver** vertex);
void DrawButtons();
void RastRect(int x1, int y1, int x2, int y2, struct color Cl);
void VerInit(struct ver** vertex, int nvr);
void Rast_Texture(char* filename, struct v2D A, struct v2D B, struct v2D C);
int Proection(char flag, struct ver* vertex, int num);
int isOutside(struct v2D A, struct v2D B, struct v2D C);
void normalizeMe(struct v3D *vec);
void viewing(double x, double xO,double y, double yO, double z, double zO, double *pxe, double *pye, double *pze);
void coeff(double rho, double theta, double phi);
void swapI(int *x, int *y);
void rotateHandler(GdkEventMotion *event, double *theta, double *phi);
void update();
void rast_tr(struct v2D A, struct v2D B, struct v2D C, struct color Cl);
void swapS(struct v2D *A, struct v2D *B);
void swap(double *x, double *y);
void on_window_destroy();
gboolean motion_notify_event_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data);
gboolean button_press_event_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean upload_button_clicked_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);

double v11, v12, v13, v21, v22, v23, v32, v33, v43, d, c1, c2, onetime = 1, Xvp_range, Yvp_range;
double eps = 1.e-5, meps = -1.e-5, oneminus = 1 - 1.e-5, oneplus = 1 + 1.e-5;

int *zbuffer;
static cairo_surface_t *surface = NULL;
struct obj *testObj;
GdkPixbuf* buff;
guchar *pixels;

int rowstride, n_channels;
static void put_pixel(int x, int y, guchar red, guchar green, guchar blue) {
    guchar* p = pixels + y * rowstride + x * n_channels;
    p[0] = red;
    p[1] = green;
    p[2] = blue;
}

static void clear_surface (void) {
    cairo_t *cr;
    cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_destroy(cr);
}



/* Create a new surface of the appropriate size to store our scribbles */
static gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
    if (surface)
        cairo_surface_destroy (surface);
    surface = gdk_window_create_similar_surface (gtk_widget_get_window(widget), CAIRO_CONTENT_COLOR, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
    clear_surface ();
    return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data) {
    cairo_set_source_surface (cr, surface, 0, 0);
    cairo_paint (cr);
    return FALSE;
}

/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */

gboolean button_press_event_cb (GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (surface == NULL)
        return FALSE;
    if (event->button == GDK_BUTTON_PRIMARY) {
        //draw_brush (widget, event->x, event->y);
    }
    else if (event->button == GDK_BUTTON_SECONDARY) {
    }
    return TRUE;
}

gboolean upload_button_clicked_cb (GtkWidget *widget, GdkEventButton *event, gpointer data) {
    return TRUE;  
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
gboolean motion_notify_event_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    static double phi = 0, theta = 0;
    if (surface == NULL)
        return FALSE;
    if (event->state & GDK_BUTTON1_MASK) {
        update();
        rotateHandler(event, &phi, &theta);
        cairo_t *cr = cairo_create(surface);
        gdk_pixbuf_fill(buff, 0x000000);
        draw_obj(buff, testObj, Red, theta, phi);
        gdk_cairo_set_source_pixbuf(cr, buff, 5, 5);
        cairo_paint(cr);
        cairo_fill (cr);
        cairo_destroy(cr);
        gtk_widget_queue_draw (widget);
    }
    return TRUE;
}

void update()
{
    int i;
    for (i = 0; i < (Xvp_range + 1) * (Yvp_range + 1); ++i)
        if ( (zbuffer + i) != NULL)
            *(zbuffer + i) = 20000;

}

void rotateHandler(GdkEventMotion *event, double *theta, double *phi)
{
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

static void close_window (void) {
    if (surface)
      cairo_surface_destroy (surface);
    gtk_main_quit ();
}

static void activate (GtkApplication *app, gpointer user_data) {
    // buff = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8, 600, 600);
    // n_channels = gdk_pixbuf_get_n_channels (buff);
    // rowstride = gdk_pixbuf_get_rowstride (buff);
    // pixels = gdk_pixbuf_get_pixels (buff);

    // //GtkBuilder *gtkBuilder = gtk_builder_new();
    // GtkBuilder *gtkBuilder = gtk_builder_new_from_file ("render.glade");
    // //gtk_builder_add_from_file(gtkBuilder,"render.glade", NULL); 
    // //gtk_builder_set_application(gtkBuilder, app);
    // gtk_builder_connect_signals(gtkBuilder, NULL);

    // GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "window1")); 
    // GtkWidget *render_area = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "render_area")); 
    // GtkWidget *upload_button = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "upload_button")); 
    // g_object_unref(G_OBJECT(gtkBuilder));
    // // GtkWidget *window = gtk_application_window_new(app);
    // // gtk_window_set_default_size(GTK_WINDOW(window), 1000, 470);
    // // gtk_window_set_title(GTK_WINDOW(window), "Drawing Area");

    // // gtk_container_set_border_width(GTK_CONTAINER (window), 8);
    // // GtkWidget *frame = gtk_frame_new(NULL);
    // // gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
    // // gtk_container_add (GTK_CONTAINER(window), frame);
    // // GtkWidget *drawing_area = gtk_drawing_area_new();
    // // gtk_widget_set_size_request(drawing_area, 100, 100);
    // // gtk_container_add(GTK_CONTAINER(frame), drawing_area);


    // g_signal_connect (window, "destroy", G_CALLBACK(close_window), NULL);
    // g_signal_connect (render_area, "draw", G_CALLBACK (draw_cb), NULL);
    // g_signal_connect (render_area,"configure-event", G_CALLBACK (configure_event_cb), NULL);
    // g_signal_connect (render_area, "motion-notify-event", G_CALLBACK (motion_notify_event_cb), NULL);
    // g_signal_connect (render_area, "button-press-event", G_CALLBACK (button_press_event_cb), NULL);
    // g_signal_connect (upload_button, "button-press-event", G_CALLBACK(upload_button_press_event_cb), NULL);
    // /* Ask to receive events the drawing area doesn't normally
    //  * subscribe to. In particular, we need to ask for the
    //  * button press and motion notify events that want to handle.
    //  */
    // gtk_widget_set_events (render_area, gtk_widget_get_events(render_area) | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    // gtk_widget_set_events (upload_button, gtk_widget_get_events(upload_button) | GDK_BUTTON_PRESS_MASK);
    // gtk_widget_show_all (window);
}

int main (int argc, char **argv)
{
    // GtkApplication *app;
    // int status;

    // testObj = (struct obj*)malloc(sizeof(struct obj));
    // read_object("african_head.obj", testObj);

    // app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    // g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    // status = g_application_run (G_APPLICATION (app), argc, argv);
    // g_object_unref (app);

    // return status;
    GtkBuilder      *builder; 
    GtkWidget       *window;
 
    gtk_init(&argc, &argv);
 
    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "render.glade", NULL);
 
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    gtk_builder_connect_signals(builder, NULL);
 
    g_object_unref(builder);
 
    gtk_widget_show(window);                
    gtk_main();
 
    return 0;
}

void on_window_destroy()
{
    gtk_main_quit();
}

void read_object(char *filename, struct obj *object)
{
    FILE *fpin;
    char string[100];

    struct ver *fvertex;
    struct ver *vertex;
    int nvr = 1;

    struct tr *triangle;
    int ntr = 0;

    fpin = fopen(filename, "r");
    while(!feof(fpin)) {
        fgets(string, sizeof(string), fpin);
        switch (string[0]) {
            case 'v':
                if (string[1] == ' ') {
                    string[strlen(string) - 1] = '\0';
                    ReadVertex(string, &fvertex, &nvr);
                }
                break;
            case 'f':
                string[strlen(string) - 1] = '\0';
                ReadFace(string, &triangle, &ntr);
                break;
            default: continue; break;
        }
    }
    VerInit(&vertex, nvr);
    object->fvertex = fvertex; object->vertex = vertex; object->ntr = ntr; object->nvr = nvr; object->triangle = triangle;
    fclose(fpin);
}

void ReadVertex(char *str, struct ver **fvertex, int *nvr)
{
    static double xmin = BIG, ymin = BIG, zmin = BIG, xmax = -BIG, ymax = -BIG, zmax = -BIG;
    char numx[15], numy[15], numz[15];
    int i = 1;
    while (str[i] == ' ')
        ++i;
    int j = 0;
    while (str[i] != ' ') {
        numx[j] = str[i];
        ++i; ++j;
    }
    double x = atof(numx) * 100;
    ++i; 
    j = 0;
    while (str[i] != ' ') {
        numy[j] = str[i];
        ++i; ++j;
    }
    double y = atof(numy) * 100;
    ++i; j = 0;
    while (str[i] != '\0') {
        numz[j] = str[i];
       ++i; ++j;
    }
    double z = atof(numz) * 100;

    if (x > xmax) xmax = x; if (x < xmin) xmin = x;
    if (y > ymax) ymax = y; if (y < ymin) ymin = y;
    if (z > zmax) zmax = z; if (z < zmin) zmin = z;

    if (*nvr == 1) {
        *fvertex = (struct ver *)malloc(2 * sizeof(struct ver));
        (*fvertex)[0].x = 0; (*fvertex)[0].y = 0; (*fvertex)[0].z = 0;
    } else
        *fvertex = (struct ver *)realloc(*fvertex, (*nvr + 1) * sizeof(struct ver));

    (*fvertex)->x = (xmin + xmax) / 2; (*fvertex)->y = (ymin + ymax) / 2; (*fvertex)->z = (zmin + zmax) / 2;
    (*fvertex + *nvr)->x = x; (*fvertex + *nvr)->y = y; (*fvertex + *nvr)->z = z;
    *nvr += 1;
}

void ReadFace(char *str, struct tr **triangle, int *ntr)
{
    char num[10], part[150][30];
    int j = 0, i = 2, k = 0, poly[30];

    while (1 == 1) {
        if (str[i] == ' ') {
            part[k][j] = '\0';
            if (str[i + 1] == '\0') break;
            ++i; ++k; j = 0;
            continue;
        } else if (str[i] == '\0') {
            part[k][j] = '\0';
            break;
        }
        part[k][j] = str[i];
        ++j; ++i;
    }

    for (i = 0; i <= k; ++i) {
        j = 0;
        while (part[i][j] != '/' && part[i][j] != '\0') {
            num[j] = part[i][j];
            ++j;
        }
        poly[i] = abs(atoi(num));
        while (j >= 0) {
            num[j] = '\0';
            --j;
        }
    }

    int i0 = 0, i1 = 1, i2 = 2, A, B, C, first = 1;
    while(i2 != k + 1) {
        A = poly[i0], B = poly[i1], C = poly[i2];
        if (*ntr == 0)
            { *triangle = malloc(sizeof(struct tr));}
        else
           *triangle = (struct tr*)realloc(*triangle, (*ntr + 1) * sizeof(struct tr));

        (*triangle + *ntr)->A = A; (*triangle + *ntr)->B = B; (*triangle + *ntr)->C = C; (*triangle + *ntr)->first = first;
        *ntr += 1; ++i1; ++i2; first = 0;
    }
}

void VerInit(struct ver **vertex, int nvr)
{   
    int i;
    *vertex = (struct ver*)malloc((nvr + 1) * sizeof(struct ver));
    for (i = 1; i <= nvr; ++i) {
        (*vertex + i)->connect = (int *)malloc(100 * sizeof(int));
        *((*vertex + i)->connect) = 0;
    }
}

void draw_obj(GdkPixbuf* buffer, struct obj *object, struct color C, double theta, double phi)
{
    struct ver **fvertex = &(object->fvertex);
    struct ver **vertex = &(object->vertex);
    int nvr = object->nvr;

    struct tr **triangle = &object->triangle;
    int ntr = object->ntr;

    double Range[4] = {BIG, -BIG, BIG, -BIG};
  //clock_t begin = clock();
    coeff(3000.0, theta, phi);
  //clock_t end = clock();
  //double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  //printf("%f spent on recouting coefficients\n", time_spent);

  //begin = clock();
    CalcVertex(*fvertex, vertex, nvr, Range);
  //end = clock();
  //time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  //printf("%f spent on CalcVertex routine\n", time_spent);

    if (onetime) {
        scale_calc(Range[0], Range[1], Range[2], Range[3]);
        extra_init();
    }
    onetime = 0;

  //begin = clock();
    CalcTr(*vertex, triangle, ntr, nvr);
  //end = clock();
  //time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  //printf("%f spent on CaltTr routine\n", time_spent);
        /* Считаем нормали к вершинам*/
       //extraCalcVertex(vertex, nvr, *triangle);

    DrawTr(*triangle, ntr, *vertex, C);
}

void coeff(double rho, double theta, double phi)
{
    double th, ph, costh, sinth, cosph, sinph, factor;

    factor = atan(1.0) / 45.0;
    th = theta * factor; ph = phi * factor;
    costh = cos(th); sinth = sin(th);
    cosph = cos(ph); sinph = sin(ph);

    v11 = -sinth; v12 = -cosph * costh; v13 = -sinph * costh;
    v21 = costh;  v22 = -cosph * sinth; v23 = -sinph * sinth;
                  v32 = sinph;          v33 = -cosph;
                                        v43 = rho;
}

void scale_calc(double Xmin, double Xmax, double Ymin, double Ymax)
{
    double fx, fy, Xcentre, Ycentre, Xvp_centre, Yvp_centre;
    double Xrange, Yrange;

    Xrange = Xmax - Xmin; Yrange = Ymax - Ymin;
    Xvp_range = Xvp_max - Xvp_min; Yvp_range = Yvp_max - Yvp_min;
    fx = Xvp_range / Xrange; fy = Yvp_range / Yrange;
    d = (fx < fy) ? fx : fy;
    Xcentre = (Xmin + Xmax) / 2; Ycentre = (Ymax + Ymin) / 2;
    Xvp_centre = (Xvp_min + Xvp_max) / 2; Yvp_centre = (Yvp_min + Yvp_max) / 2;
    c1 = Xvp_centre - d * Xcentre; c2 = Yvp_centre - d * Ycentre;
}

void CalcVertex(struct ver* fvertex, struct ver** vertex, int nvr, double *Range)
{
    int i;
    double x, y, z, xe, ye, ze, X, Y;
    double xO = fvertex[0].x, yO = fvertex[0].y, zO = fvertex[0].z;
    for (i = 1; i <= nvr; ++i) {
        x = (fvertex + i)->x; y = (fvertex + i)->y; z = (fvertex + i)->z;

        //преобразуем координаты (через матрицу поворота)
        viewing(x,xO, y,yO, z,zO, &xe, &ye, &ze);
        if (ze <= eps)
            continue;

        //находим max и min для масштабирования в наше окно вывода
        X = xe / ze; Y = ye / ze;

        if (X < Range[0]) Range[0] = X; if (X > Range[1]) Range[1] = X;   //Range = {Xmin, Xmax, Ymin, Ymax}
        if (Y < Range[2]) Range[2] = Y; if (Y > Range[3]) Range[3] = Y;

        (*vertex + i)->x = xe; (*vertex + i)->y = ye; (*vertex + i)->z = ze;
  if ((*vertex + i) == NULL)
      printf("some kind of error\n");
    }
}

void extra_init() {

    int i;
    zbuffer = (int *)malloc((Xvp_range + 1) * (Yvp_range + 1) * sizeof(int));
    for (i = 0; i < (Xvp_range + 1) * (Yvp_range + 1); ++i)
            *(zbuffer + i) = 20000;
}

void CalcTr(struct ver* vertex, struct tr **triangle, int ntr, int nvr) {
    double xA, yA, zA, xB, yB, zB, xC, yC, zC, a, b, c, h, r;
    int i, j, A, B, C;
    for (i = 0; i < ntr; ++i) {
        A = (*triangle + i)->A; B = (*triangle + i)->B; C = (*triangle + i)->C;
        /*После выполнения функции RotateObj / MoveObj обьект, видимо, немного повреждается
        и некоторые значения номеров треугольников очень сильно меняются
        поэтому необходима проверка их соотвествия действительности, иначе - SegFault*/
        if (A > nvr || A < 0 || B > nvr || B < 0 || C > nvr || C < 0) {
              continue;
        }
        if ((vertex + A) == NULL || (vertex + B) == NULL || (vertex + C) == NULL) {
          continue;
        }

        if ((*triangle + i)->first == 1) {
            xA = (vertex + A)->x; yA = (vertex + A)->y; zA = (vertex + A)->z;
            xB = (vertex + B)->x; yB = (vertex + B)->y; zB = (vertex + B)->z;
            xC = (vertex + C)->x; yC = (vertex + C)->y; zC = (vertex + C)->z;

            a = yA * (zB - zC) - yB * (zA - zC) + yC * (zA - zB);
            b = -(xA * (zB - zC) - xB * (zA - zC) + xC * (zA - zB));
            c =  xA * (yB - yC) - xB * (yA - yC) + xC * (yA - yB);
            h = xA * (yB * zC - yC * zB) - xB * (yA * zC - yC * zA) + xC * (yA * zB - yB * zA);

            r = sqrt(a*a + b*b + c*c);
            a /= r; b /= r; c /= r; h /= r;

            (*triangle + i)->a = a; (*triangle + i)->b = b; (*triangle + i)->c = c;
            (*triangle + i)->h = h;

        } else {
            j = i;
            while ((*triangle + j)->first != 1 )
              --j;
            (*triangle + i)->a = (*triangle + j)->a; (*triangle + i)->b = (*triangle + j)->b; (*triangle + i)->c = (*triangle + j)->c;
            (*triangle + i)->h = (*triangle + j)->h;
        }
    }
}

void DrawTr(struct tr* triangle, int ntr, struct ver* vertex, struct color Cl) {
    int i, XA, YA, XB, YB, XC, YC, A, B, C;
    double a, b, c, h, intensity;
    normalizeMe(&lightdir);
    for (i = 0; i < ntr; ++i) {
        a = triangle[i].a; b = triangle[i].b; c = triangle[i].c; h = triangle[i].h;
        A = triangle[i].A; B = triangle[i].B; C = triangle[i].C;

        intensity = lightdir.x * a + lightdir.y * b + lightdir.z * c;
        if (h > 0 && intensity > 0) {
            struct color rgb = {intensity * Cl.r, intensity * Cl.g, intensity * Cl.b};

            if (A > ntr || B > ntr || C > ntr)
                continue;
            XA = Proection('x', vertex, A); YA = Proection('y', vertex, A);
            XB = Proection('x', vertex, B); YB = Proection('y', vertex, B);
            XC = Proection('x', vertex, C); YC = Proection('y', vertex, C);

            struct v2D vA = {XA, YA, vertex[A].z};
            struct v2D vB = {XB, YB, vertex[B].z};
            struct v2D vC = {XC, YC, vertex[C].z};

            rast_tr(vA,vB,vC,rgb);
            //rast_line(vA,vB,vC,rgb);
        }
    }
}

void rast_line(struct v2D A, struct v2D B, struct v2D C, struct color Cl)
{
    line(A.x, A.y, B.x, B.y, Cl, CROP); line(B.x, B.y, C.x, C.y, Cl, CROP); line(C.x, C.y, A.x, A.y, Cl, CROP);
}

void rast_tr(struct v2D A, struct v2D B, struct v2D C, struct color Cl) {
    if (A.y == B.y && A.y == C.y) 
      return;
    if (A.y > B.y) swapS(&A,&B);
    if (A.y > C.y) swapS(&A,&C);
    if (B.y > C.y) swapS(&B,&C);

    int total_height = C.y - A.y, i, second_half, j, idx, segment_height;
    float alpha, beta, phi;
    double xI, yI, zI, xJ, yJ, zJ, Pz;
    long double time_spent_on_calculating = 0, time_spent_on_drawing = 0;
    for (i = 0; i <= total_height; ++i) {
        second_half = i > B.y - A.y || B.y == A.y;
        segment_height = second_half ? C.y - B.y : B.y - A.y;

        alpha = (float)(i) / total_height;
        beta = (float)(i - (second_half ? B.y - A.y : 0)) / segment_height;

        xI = alpha * C.x + A.x * (1 - alpha);
        yI = alpha * C.y + A.y * (1 - alpha);
        zI = alpha * C.depth + A.depth * (1 - alpha);

        xJ = second_half ? (beta * C.x + B.x * (1 - beta)) : beta * B.x + A.x * (1 - beta);
        yJ = second_half ? (beta * C.y + B.y * (1 - beta)) : beta * B.y + A.y * (1 - beta);
        zJ = second_half ? (beta * C.depth + B.depth * (1 - beta)) : beta * B.depth + A.depth * (1 - beta);

        if (xI > xJ) {
            swap(&xI, &xJ); swap(&yI, &yJ);
        }

        for (j = xI; j <= xJ; ++j) {
            if (j <= (Xvp_min) || j >= (Xvp_max) || (A.y + i) <= (Yvp_min) || (A.y + i) >= (Yvp_max) ) //будем считать, что окно вывода [50 350 50 350]
                continue;
            phi = xJ == xI ? 1. : (float)(j-xI) / (float)(xJ-xI);
            Pz = zI + (zJ - zI) * phi;
            //idx = Px + 320 * Py;
            idx = j + Yvp_range * (A.y + i);
            if ((zbuffer + idx) == NULL || idx < 0 || idx > (Xvp_range + 1) * (Yvp_range + 1)) { 
                continue; 
            } //idx иногда принимает весьма странные значения
            else if (*(zbuffer + idx) > Pz && *(zbuffer + idx) <= 20000 && *(zbuffer + idx) > -1000) {
                //!*(zbuffer + idx) <= 20000 очень важный по-видимому момент(нет, строчка выше - вот важный момент), без него SegFault выдавало
                drawpix(j, A.y + i, Cl);
                *(zbuffer + idx) = Pz;
            }
        }
    }
}

void swapS(struct v2D *A, struct v2D *B) {
    int xaux, yaux;
    xaux = A->x;
    A->x = B->x;
    B->x = xaux;

    yaux = A->y;
    A->y = B->y;
    B->y = yaux;

    double aux;
    aux = A->depth;
    A->depth = B->depth;
    B->depth = aux;
}

void swap(double *x, double *y) {
    double aux;

    aux = *x;
    *x = *y;
    *y = aux;
}

void line(int x0, int y0, int x1, int y1, struct color Cl, int flag)
{
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
                drawpix(y, x, Cl);
            else if (x > Xvp_min && x < Xvp_max && y > Yvp_min && y < Yvp_max)
                drawpix(x, y, Cl);
        if (flag == NOCROP) 
      if (steep)
               drawpix(y, x, Cl);
            else 
               drawpix(x, y, Cl);
    }
}

void drawpix(int x, int y, struct color clr) {
    put_pixel(x, y, clr.r, clr.g, clr.b);
}

int Proection(char flag, struct ver* vertex, int num) {
    switch (flag) {
    case 'x': 
      return (int)( vertex[num].x * d / vertex[num].z + c1);
        case 'y': 
          return (int)( vertex[num].y * d / vertex[num].z + c2);
    default: 
      return 1;
    }
}

void normalizeMe(struct v3D *vec) {
    double r;
    r = sqrt(sqr(vec->x) + sqr(vec->y) + sqr(vec->z));
    vec->x /= r; vec->y /= r; vec->z /= r;
}

void viewing(double x, double xO,double y, double yO, double z, double zO, double *pxe, double *pye, double *pze) {
    *pxe = (v11 * (x - xO) + v21 * (y - yO)) + xO;
    *pye = (v12 * (x - xO) + v22 * (y - yO) + v32 * (z - zO)) + yO;
    *pze = (v13 * (x - xO)+ v23 * (y - yO) + v33 * (z - zO) + v43) + zO;
}

void swapI(int *x, int *y) {
    int aux;
    aux = *x;
    *x = *y;
    *y = aux;
}

