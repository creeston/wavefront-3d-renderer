#include "renderer.h"
#include "utils.h"
#include <string.h>
#include <assert.h>

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
double v11, v12, v13, v21, v22, v23, v32, v33, v43;
double d;
double initial_d = 0;
double c1, c2;
double x_viewpoint_range, y_viewpoint_range;

int *zbuffer;
struct obj *previsouly_rendered_object;

#define M_PI 3.14159265358979323846
#define BIG_INT 2147483647
#define EPS 1.e-5
#define M_EPS -1.e-5
#define ONE_PLUS 1 + 1.e-5
#define ONE_MINUS 1 - 1.e-5

#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)
#define RAD_TO_DEG(rad) ((rad) * 180.0 / M_PI)
#define SQR(x) ((x) * (x))

void rasterize_triangle(struct vertice_2d A, struct vertice_2d B, struct vertice_2d C, struct color color);
void render_triangles(struct triangle *triangle, int ntr, struct vertice *vertex, int number_of_vertices, struct color color);
void render_triangle(struct triangle triangle, struct vertice *vertices, int number_of_vertices, struct color color);
void initialize_z_buffer();
void calculate_vertexes(struct vertice *fvertex, struct vertice **vertex, int nvr, double *Range);
void calculate_scale(double Xmin, double Xmax, double Ymin, double Ymax);
void calculate_triangles(struct vertice *vertex, struct triangle **triangle, int ntr, int nvr);
int project_x_coordinate(struct vertice vertex);
int project_y_coordinate(struct vertice vertex);
struct vertice_2d project_vertex(struct vertice vertex);
void normalize_vector(struct vector_3d *vec);
void viewing(double x, double xO, double y, double yO, double z, double zO, double *pxe, double *pye, double *pze);
void calculate_rotation_matrix(double rho, double theta, double phi);
void reset_z_buffer();
void swap_vertices(struct vertice_2d *A, struct vertice_2d *B);

void initialize_renderer()
{
    x_viewpoint_range = x_viewpoint_max - x_viewpoint_min;
    y_viewpoint_range = y_viewpoint_max - y_viewpoint_min;
    initialize_z_buffer();
}

void render_obj(struct obj *object, struct color color, double theta, double phi, double rho)
{
    reset_z_buffer();

    struct vertice **fvertices = &(object->fvertex);
    struct vertice **vertices = &(object->vertex);
    struct triangle **triangles = &object->triangle;
    int number_of_vertices = object->nvr;
    int number_of_triangles = object->ntr;
    double range[4] = {BIG, -BIG, BIG, -BIG};
    calculate_rotation_matrix(rho, theta, phi);
    calculate_vertexes(*fvertices, vertices, number_of_vertices, range);

    if (previsouly_rendered_object != object)
    {
        previsouly_rendered_object = object;
        calculate_scale(range[0], range[1], range[2], range[3]);
    }

    calculate_triangles(*vertices, triangles, number_of_triangles, number_of_vertices);
    render_triangles(*triangles, number_of_triangles, *vertices, number_of_vertices, color);
}

void render_triangles(struct triangle *triangles, int number_of_triangles, struct vertice *vertices, int number_of_vertices, struct color color)
{
    normalize_vector(&light_direction);
    for (int i = 0; i < number_of_triangles; ++i)
    {
        render_triangle(triangles[i], vertices, number_of_vertices, color);
    }
}

void render_triangle(struct triangle triangle, struct vertice *vertices, int number_of_vertices, struct color color)
{
    double a = triangle.a;
    double b = triangle.b;
    double c = triangle.c;
    double h = triangle.h;
    int vertex_a_idx = triangle.vertex_a;
    int vertex_b_idx = triangle.vertex_b;
    int vertex_c_idx = triangle.vertex_c;

    if (h < 0) // TODO: Consider necessity of this condition
    {
        return;
    }

    if (vertex_a_idx > number_of_vertices || vertex_b_idx > number_of_vertices || vertex_c_idx > number_of_vertices) // Improperly configured object. TODO: Understand when this happens.
    {
        printf("Invalid vertex index\n");
        return;
    }

    double intensity = light_direction.x * a + light_direction.y * b + light_direction.z * c;

    if (intensity < 0) //  triangle is invisible (not facing the light) TODO: consider the necessity of this condition
    {
        return;
    }

    struct color intensity_modified_color = {intensity * color.r, intensity * color.g, intensity * color.b};
    struct vertice_2d vertex_a = project_vertex(vertices[vertex_a_idx]);
    struct vertice_2d vertex_b = project_vertex(vertices[vertex_b_idx]);
    struct vertice_2d vertex_c = project_vertex(vertices[vertex_c_idx]);

    rasterize_triangle(vertex_a, vertex_b, vertex_c, intensity_modified_color);
}

void initialize_z_buffer()
{
    if (zbuffer != NULL)
    {
        free(zbuffer);
    }

    int buffer_length = (x_viewpoint_range + 1) * (y_viewpoint_range + 1);
    zbuffer = (int *)malloc(buffer_length * sizeof(int));
    for (int i = 0; i < buffer_length; i++)
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
        if (ze <= EPS)
        {
            continue;
        }

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
        A = (*triangle + i)->vertex_a;
        B = (*triangle + i)->vertex_b;
        C = (*triangle + i)->vertex_c;

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

            int pixel_index = x + x_viewpoint_range * y;
            assert(pixel_index >= 0);

            double phi = xJ == xI ? 1. : (float)(x - xI) / (float)(xJ - xI); // ranges from 0 to 1 depending on current x
            int pixel_z_value = zI + (zJ - zI) * phi;                        // interpolate z value
            int current_z_value = *(zbuffer + pixel_index);

            if (current_z_value <= pixel_z_value) // then pixel is behind the current one
            {
                continue;
            }

            draw_pixel(x, y, color);
            *(zbuffer + pixel_index) = pixel_z_value;
        }
    }
}

struct vertice_2d project_vertex(struct vertice vertex)
{
    struct vertice_2d v;
    v.x = project_x_coordinate(vertex);
    v.y = project_y_coordinate(vertex);
    v.depth = vertex.z;
    return v;
}

int project_x_coordinate(struct vertice vertex)
{
    return (int)(vertex.x * d / vertex.z + c1);
}

int project_y_coordinate(struct vertice vertex)
{
    return (int)(vertex.y * d / vertex.z + c2);
}

void normalize_vector(struct vector_3d *vec)
{
    double r;
    r = sqrt(SQR(vec->x) + SQR(vec->y) + SQR(vec->z));
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

void calculate_rotation_matrix(double rho, double theta, double phi)
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
    int buffer_length = (x_viewpoint_range + 1) * (y_viewpoint_range + 1);
    for (int i = 0; i < buffer_length; i++)
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
