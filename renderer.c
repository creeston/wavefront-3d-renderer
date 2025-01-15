#include "renderer.h"
#include "utils.h"
#include <string.h>

#define sqr(x) ((x) * (x))

struct vertice_2d
{
    int x, y;
    float depth;
};

struct vector_3d
{
    double x, y, z;
};

struct vector_3d light_direction = {0, 0, 1};
double v11, v12, v13, v21, v22, v23, v32, v33, v43, d, c1, c2, onetime = 1;
double initial_d = 0;
double eps = 1.e-5, meps = -1.e-5, oneminus = 1 - 1.e-5, oneplus = 1 + 1.e-5;
int *zbuffer;
double x_viewpoint_range, y_viewpoint_range;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BIG_INT 2147483647

#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)
#define RAD_TO_DEG(rad) ((rad) * 180.0 / M_PI)

void rasterize_triangle(struct vertice_2d A, struct vertice_2d B, struct vertice_2d C, struct color Cl);
void render_triangles(struct triangle *triangle, int ntr, struct vertice *vertex, struct color Cl);
void initialize_z_buffer();
void calculate_vertexes(struct vertice *fvertex, struct vertice **vertex, int nvr, double *Range);
void calculate_scale(double Xmin, double Xmax, double Ymin, double Ymax);
void calculate_triangles(struct vertice *vertex, struct triangle **triangle, int ntr, int nvr);
int proect_coordinate(char flag, struct vertice *vertex, int num);
void normalize_vector(struct vector_3d *vec);
void viewing(double x, double xO, double y, double yO, double z, double zO, double *pxe, double *pye, double *pze);
void count_coefficients(double rho, double theta, double phi);
void reset_z_buffer();

void swap_vertices(struct vertice_2d *A, struct vertice_2d *B)
{
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

void initialize_renderer()
{
    x_viewpoint_range = x_viewpoint_max - x_viewpoint_min;
    y_viewpoint_range = y_viewpoint_max - y_viewpoint_min;
    initialize_z_buffer();
}

void draw_obj(struct obj *object, struct color color, double theta, double phi, double rho)
{
    reset_z_buffer();

    struct vertice **fvertex = &(object->fvertex);
    struct vertice **vertex = &(object->vertex);
    int nvr = object->nvr;
    struct triangle **triangle = &object->triangle;
    int ntr = object->ntr;
    double range[4] = {BIG, -BIG, BIG, -BIG};
    count_coefficients(rho, theta, phi);
    calculate_vertexes(*fvertex, vertex, nvr, range);

    if (onetime)
    {
        calculate_scale(range[0], range[1], range[2], range[3]);
    }
    onetime = 0;

    calculate_triangles(*vertex, triangle, ntr, nvr);
    render_triangles(*triangle, ntr, *vertex, color);
}

void render_triangles(struct triangle *triangles, int ntr, struct vertice *vertices, struct color color)
{
    normalize_vector(&light_direction);
    for (int i = 0; i < ntr; ++i)
    {
        double a = triangles[i].a;
        double b = triangles[i].b;
        double c = triangles[i].c;
        double h = triangles[i].h;
        int A = triangles[i].A;
        int B = triangles[i].B;
        int C = triangles[i].C;

        double intensity = light_direction.x * a + light_direction.y * b + light_direction.z * c;
        if (h > 0 && intensity > 0)
        {
            struct color rgb = {intensity * color.r, intensity * color.g, intensity * color.b};

            if (A > ntr || B > ntr || C > ntr)
            {
                continue;
            }

            int XA = proect_coordinate('x', vertices, A);
            int YA = proect_coordinate('y', vertices, A);
            int XB = proect_coordinate('x', vertices, B);
            int YB = proect_coordinate('y', vertices, B);
            int XC = proect_coordinate('x', vertices, C);
            int YC = proect_coordinate('y', vertices, C);

            struct vertice_2d vA = {XA, YA, vertices[A].z};
            struct vertice_2d vB = {XB, YB, vertices[B].z};
            struct vertice_2d vC = {XC, YC, vertices[C].z};

            rasterize_triangle(vA, vB, vC, rgb);
        }
    }
}

void initialize_z_buffer()
{
    if (zbuffer != NULL)
    {
        free(zbuffer);
    }

    zbuffer = (int *)malloc((x_viewpoint_range + 1) * (y_viewpoint_range + 1) * sizeof(int));
    for (int i = 0; i < (x_viewpoint_range + 1) * (y_viewpoint_range + 1); i++)
    {
        *(zbuffer + i) = BIG_INT;
    }
}

void calculate_vertexes(struct vertice *fvertex, struct vertice **vertex, int nvr, double *Range)
{
    int i;
    double x, y, z, xe, ye, ze, X, Y;
    double xO = fvertex[0].x, yO = fvertex[0].y, zO = fvertex[0].z;
    for (i = 1; i <= nvr; ++i)
    {
        x = (fvertex + i)->x;
        y = (fvertex + i)->y;
        z = (fvertex + i)->z;

        viewing(x, xO, y, yO, z, zO, &xe, &ye, &ze);
        if (ze <= eps)
            continue;

        X = xe / ze;
        Y = ye / ze;

        if (X < Range[0])
            Range[0] = X;
        if (X > Range[1])
            Range[1] = X;
        if (Y < Range[2])
            Range[2] = Y;
        if (Y > Range[3])
            Range[3] = Y;

        (*vertex + i)->x = xe;
        (*vertex + i)->y = ye;
        (*vertex + i)->z = ze;
        if ((*vertex + i) == NULL)
        {
            printf("some kind of error\n");
        }
    }
}

void calculate_scale(double Xmin, double Xmax, double Ymin, double Ymax)
{
    double fx, fy, Xcentre, Ycentre;
    double Xrange, Yrange;

    Xrange = Xmax - Xmin;
    Yrange = Ymax - Ymin;

    fx = x_viewpoint_range / Xrange;
    fy = y_viewpoint_range / Yrange;
    d = (fx < fy) ? fx : fy;
    initial_d = d;
    Xcentre = (Xmin + Xmax) / 2;
    Ycentre = (Ymax + Ymin) / 2;
    double x_viewpoint_center = (x_viewpoint_min + x_viewpoint_max) / 2;
    double y_viewpoint_center = (y_viewpoint_min + y_viewpoint_max) / 2;
    c1 = x_viewpoint_center - d * Xcentre;
    c2 = y_viewpoint_center - d * Ycentre;
}

void calculate_triangles(struct vertice *vertex, struct triangle **triangle, int ntr, int nvr)
{
    double xA, yA, zA, xB, yB, zB, xC, yC, zC, a, b, c, h, r;
    int i, j, A, B, C;
    for (i = 0; i < ntr; ++i)
    {
        A = (*triangle + i)->A;
        B = (*triangle + i)->B;
        C = (*triangle + i)->C;

        if (A > nvr || A < 0 || B > nvr || B < 0 || C > nvr || C < 0)
        {
            continue;
        }

        if ((vertex + A) == NULL || (vertex + B) == NULL || (vertex + C) == NULL)
        {
            continue;
        }

        if ((*triangle + i)->first == 1)
        {
            xA = (vertex + A)->x;
            yA = (vertex + A)->y;
            zA = (vertex + A)->z;
            xB = (vertex + B)->x;
            yB = (vertex + B)->y;
            zB = (vertex + B)->z;
            xC = (vertex + C)->x;
            yC = (vertex + C)->y;
            zC = (vertex + C)->z;

            a = yA * (zB - zC) - yB * (zA - zC) + yC * (zA - zB);
            b = -(xA * (zB - zC) - xB * (zA - zC) + xC * (zA - zB));
            c = xA * (yB - yC) - xB * (yA - yC) + xC * (yA - yB);
            h = xA * (yB * zC - yC * zB) - xB * (yA * zC - yC * zA) + xC * (yA * zB - yB * zA);

            r = sqrt(a * a + b * b + c * c);
            a /= r;
            b /= r;
            c /= r;
            h /= r;

            (*triangle + i)->a = a;
            (*triangle + i)->b = b;
            (*triangle + i)->c = c;
            (*triangle + i)->h = h;
        }
        else
        {
            j = i;
            while ((*triangle + j)->first != 1)
                --j;
            (*triangle + i)->a = (*triangle + j)->a;
            (*triangle + i)->b = (*triangle + j)->b;
            (*triangle + i)->c = (*triangle + j)->c;
            (*triangle + i)->h = (*triangle + j)->h;
        }
    }
}

void rasterize_triangle(struct vertice_2d v1, struct vertice_2d v2, struct vertice_2d v3, struct color color)
{
    if (v1.y == v2.y && v1.y == v3.y)
    {
        return;
    }
    if (v1.y > v2.y)
    {
        swap_vertices(&v1, &v2);
    }
    if (v1.y > v3.y)
    {
        swap_vertices(&v1, &v3);
    }
    if (v2.y > v3.y)
    {
        swap_vertices(&v2, &v3);
    }

    int total_height = v3.y - v1.y;
    for (int i = 0; i <= total_height; ++i)
    {
        int second_half = i > v2.y - v1.y || v2.y == v1.y;
        int segment_height = second_half ? v3.y - v2.y : v2.y - v1.y;

        double alpha = (float)(i) / total_height;
        double beta = (float)(i - (second_half ? v2.y - v1.y : 0)) / segment_height;

        double xI = alpha * v3.x + v1.x * (1 - alpha);
        double yI = alpha * v3.y + v1.y * (1 - alpha);
        double zI = alpha * v3.depth + v1.depth * (1 - alpha);

        double xJ = second_half ? (beta * v3.x + v2.x * (1 - beta)) : beta * v2.x + v1.x * (1 - beta);
        double yJ = second_half ? (beta * v3.y + v2.y * (1 - beta)) : beta * v2.y + v1.y * (1 - beta);
        double zJ = second_half ? (beta * v3.depth + v2.depth * (1 - beta)) : beta * v2.depth + v1.depth * (1 - beta);

        if (xI > xJ)
        {
            swap_doubles(&xI, &xJ);
            swap_doubles(&yI, &yJ);
        }

        int y = v1.y + i;
        for (int x = xI; x <= xJ; ++x)
        {
            if (x <= x_viewpoint_min || x >= x_viewpoint_max || y <= y_viewpoint_min || y >= y_viewpoint_max)
            {
                continue;
            }

            int pixel_index = x + y_viewpoint_range * y;
            if (pixel_index < 0)
            {
                printf("Error: pixel index is negative\n");
                continue;
            }

            if ((zbuffer + pixel_index) == NULL)
            {
                printf("Error: zbuffer is NULL at index %d\n", pixel_index);
                continue;
            }

            double phi = xJ == xI ? 1. : (float)(x - xI) / (float)(xJ - xI); // ranges from 0 to 1 depending on current x
            int pixel_z_value = zI + (zJ - zI) * phi;                        // interpolate z value
            int current_z_value = *(zbuffer + pixel_index);

            // For some reason, the z-buffer is not working properly, resulting in skipped polygons
            if (current_z_value <= pixel_z_value)
            {
                // Pixel is behind the current pixel, therefore skip it
                continue;
            }

            draw_pixel(x, y, color);
            *(zbuffer + pixel_index) = pixel_z_value;
        }
    }
}

int proect_coordinate(char flag, struct vertice *vertex, int num)
{
    switch (flag)
    {
    case 'x':
        return (int)(vertex[num].x * d / vertex[num].z + c1);
    case 'y':
        return (int)(vertex[num].y * d / vertex[num].z + c2);
    default:
        return 1;
    }
}

void normalize_vector(struct vector_3d *vec)
{
    double r;
    r = sqrt(sqr(vec->x) + sqr(vec->y) + sqr(vec->z));
    vec->x /= r;
    vec->y /= r;
    vec->z /= r;
}

void viewing(double x, double xO, double y, double yO, double z, double zO, double *pxe, double *pye, double *pze)
{
    *pxe = (v11 * (x - xO) + v21 * (y - yO)) + xO;
    *pye = (v12 * (x - xO) + v22 * (y - yO) + v32 * (z - zO)) + yO;
    *pze = (v13 * (x - xO) + v23 * (y - yO) + v33 * (z - zO) + v43) + zO;
}

void count_coefficients(double rho, double theta, double phi)
{
    double th, ph, costh, sinth, cosph, sinph, factor;

    th = DEG_TO_RAD(theta);
    ph = DEG_TO_RAD(phi);
    costh = cos(th);
    sinth = sin(th);
    cosph = cos(ph);
    sinph = sin(ph);

    v11 = -sinth;
    v12 = -cosph * costh;
    v13 = -sinph * costh;
    v21 = costh;
    v22 = -cosph * sinth;
    v23 = -sinph * sinth;
    v32 = sinph;
    v33 = -cosph;
    v43 = rho;
}

void reset_z_buffer()
{
    for (int i = 0; i < (x_viewpoint_range + 1) * (y_viewpoint_range + 1); i++)
    {
        *(zbuffer + i) = BIG_INT;
    }
}

void move_obj_relative(struct obj *object, int dx, int dy, int dz)
{
    for (int i = 1; i <= object->nvr; ++i)
    {
        object->fvertex[i].x += dx;
        object->fvertex[i].y += dy;
        object->fvertex[i].z += dz;
    }
}

void move_obj_absolute(struct obj *object, int x, int y, int z)
{
    int dx = x - object->fvertex[0].x;
    int dy = y - object->fvertex[0].y;
    int dz = z - object->fvertex[0].z;
    for (int i = 1; i <= object->nvr; ++i)
    {
        object->fvertex[i].x += dx;
        object->fvertex[i].y += dy;
        object->fvertex[i].z += dz;
    }
}

void change_scale(float scale)
{
    d = initial_d * scale;
}