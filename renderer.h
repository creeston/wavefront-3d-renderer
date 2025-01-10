#include "canvas_drawing.h"

#define BIG 1e30

struct ver
{
    double x, y, z;
    int *connect;
    double a, b, c;
};

struct tr
{
    int A, B, C;
    double a, b, c, h;
    int first;
};

struct obj
{
    struct ver *fvertex, *vertex;
    int nvr;
    struct tr *triangle;
    int ntr;
    int n;
};

struct v2D
{
    int x, y;
    float depth;
};

struct v3D
{
    double x, y, z;
};

void read_object(char *filename, struct obj *object);
void draw_obj(struct obj *object, struct color C, double theta, double phi);
void draw_objects(struct obj **object, int objects_amount, struct color C, double theta, double phi, double rho);
void draw_triangle(struct tr *triangle, int ntr, struct ver *vertex, struct color Cl);
void init_z_buffer();
void calculate_vertexes(struct ver *fvertex, struct ver **vertex, int nvr, double *Range);
void calculate_scale(double Xmin, double Xmax, double Ymin, double Ymax);
void calculate_triangles(struct ver *vertex, struct tr **triangle, int ntr, int nvr);
void rast_triangles(struct v2D A, struct v2D B, struct v2D C, struct color Cl);
void read_vertex(char *str, struct ver **fvertex, int *nvr);
void read_face(char *str, struct tr **triangle, int *ntr);
void init_vertexes(struct ver **vertex, int nvr);
int proect_coordinate(char flag, struct ver *vertex, int num);
void normalize_vector(struct v3D *vec);
void viewing(double x, double xO, double y, double yO, double z, double zO, double *pxe, double *pye, double *pze);
void count_coefficients(double rho, double theta, double phi);
void update_z_buffer();
void rast_line(struct v2D A, struct v2D B, struct v2D C, struct color Cl);
void swapS(struct v2D *A, struct v2D *B);
void move_obj_relative(struct obj *object, int dx, int dy, int dz);
void move_obj_absolute(struct obj *object, int x, int y, int z);

extern struct color Red;
extern struct color Black;
extern int Background;
extern struct v3D lightdir;
extern double v11, v12, v13, v21, v22, v23, v32, v33, v43, d, initial_d, c1, c2, onetime, Xvp_range, Yvp_range, eps, meps, oneminus, oneplus;
extern int *zbuffer;