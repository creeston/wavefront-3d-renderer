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

struct triangle_vector
{
    double a, b, c, h;
};

struct vector_3d light_direction = {0, 0, 1};
double v11, v12, v13, v21, v22, v23, v32, v33, v43;
double d;
double initial_d = 0;
double c1, c2;
double x_viewpoint_range, y_viewpoint_range;

int *zbuffer;
struct obj *previsouly_rendered_object;

double theta, phi, rho = 3000;

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
void render_triangles(struct obj_triangle *triangles, struct triangle_vector *triangle_vectors, int number_of_triangles, struct vector_3d *vertices, int number_of_vertices, struct color color);
void render_triangle(struct obj_triangle triangle, struct triangle_vector triangle_vector, struct vector_3d *vertices, int number_of_vertices, struct color color);
void initialize_z_buffer();
struct vector_3d *calculate_rotated_vertices(struct obj_vertex *object_vertices, int number_of_vertices);
void calculate_scale(struct vector_3d *vertices, int number_of_vertices);
struct triangle_vector *calculate_triangles(struct vector_3d *vertex, struct obj_triangle *triangle, int ntr, int nvr);
int project_x_coordinate(struct vector_3d vertex);
int project_y_coordinate(struct vector_3d vertex);
struct vertice_2d project_vertex(struct vector_3d vertex);
void normalize_vector(struct vector_3d *vec);
struct vector_3d apply_rotation_matrix(struct obj_vertex vertice, struct obj_vertex original_vertice);
void reset_z_buffer();
void swap_vertices(struct vertice_2d *A, struct vertice_2d *B);
void calculate_rotation_matrix();

void initialize_renderer()
{
    x_viewpoint_range = x_viewpoint_max - x_viewpoint_min;
    y_viewpoint_range = y_viewpoint_max - y_viewpoint_min;
    initialize_z_buffer();
}

void set_rotation_angles(double theta_value, double phi_value, double rho_value)
{
    theta = theta_value;
    phi = phi_value;
    rho = rho_value;

    calculate_rotation_matrix();
}

void render(struct obj *object, struct color color)
{
    struct obj_vertex *object_vertices = object->vertices;
    struct obj_triangle *object_triangles = object->triangles;
    int number_of_vertices = object->number_of_vertices;
    int number_of_triangles = object->number_of_triangles;

    struct vector_3d *vertices = calculate_rotated_vertices(object_vertices, number_of_vertices);
    struct triangle_vector *triangle_vectors = calculate_triangles(vertices, object_triangles, number_of_triangles, number_of_vertices);

    if (previsouly_rendered_object != object)
    {
        previsouly_rendered_object = object;
        calculate_scale(vertices, number_of_vertices);
    }

    render_triangles(object_triangles, triangle_vectors, number_of_triangles, vertices, number_of_vertices, color);

    free(vertices);
    free(triangle_vectors);
}

void render_triangles(struct obj_triangle *triangles, struct triangle_vector *triangle_vectors, int number_of_triangles, struct vector_3d *vertices, int number_of_vertices, struct color color)
{
    reset_z_buffer();
    normalize_vector(&light_direction);
    for (int i = 0; i < number_of_triangles; ++i)
    {
        render_triangle(triangles[i], triangle_vectors[i], vertices, number_of_vertices, color);
    }
}

void render_triangle(struct obj_triangle triangle, struct triangle_vector triangle_vector, struct vector_3d *vertices, int number_of_vertices, struct color color)
{
    double a = triangle_vector.a;
    double b = triangle_vector.b;
    double c = triangle_vector.c;
    double h = triangle_vector.h;

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

struct vector_3d *calculate_rotated_vertices(struct obj_vertex *fvertices, int number_of_vertices)
{
    struct vector_3d *vertices = (struct vector_3d *)malloc((number_of_vertices + 1) * sizeof(struct vector_3d));
    double xO = fvertices[0].x, yO = fvertices[0].y, zO = fvertices[0].z;
    for (int i = 1; i <= number_of_vertices; ++i)
    {
        struct vector_3d rotated_vertex = apply_rotation_matrix(fvertices[i], fvertices[0]);
        if (rotated_vertex.z <= EPS)
        {
            continue;
        }

        vertices[i].x = rotated_vertex.x;
        vertices[i].y = rotated_vertex.y;
        vertices[i].z = rotated_vertex.z;
    }
    return vertices;
}

void calculate_scale(struct vector_3d *vertices, int number_of_vertices)
{
    double x_min = BIG_INT, x_max = -BIG_INT, y_min = BIG_INT, y_max = -BIG_INT;

    for (int i = 1; i <= number_of_vertices; i++)
    {
        struct vector_3d rotated_vertex = vertices[i];
        double x = rotated_vertex.x / rotated_vertex.z;
        double y = rotated_vertex.y / rotated_vertex.z;

        if (x < x_min)
        {
            x_min = x;
        }
        if (x > x_max)
        {
            x_max = x;
        }
        if (y < y_min)
        {
            y_min = y;
        }
        if (y > y_max)
        {
            y_max = y;
        }
    }

    double x_range = x_max - x_min;
    double y_range = y_max - y_min;

    double fx = x_viewpoint_range / x_range;
    double fy = y_viewpoint_range / y_range;
    double x_center = (x_min + x_max) / 2;
    double y_center = (y_max + y_min) / 2;
    double x_viewpoint_center = (x_viewpoint_min + x_viewpoint_max) / 2;
    double y_viewpoint_center = (y_viewpoint_min + y_viewpoint_max) / 2;

    d = (fx < fy) ? fx : fy;
    initial_d = d;
    c1 = x_viewpoint_center - d * x_center;
    c2 = y_viewpoint_center - d * y_center;
}

struct triangle_vector *calculate_triangles(struct vector_3d *vertices, struct obj_triangle *triangles, int number_of_triangles, int number_of_vertices)
{
    struct triangle_vector *triangle_vectors = (struct triangle_vector *)malloc(number_of_triangles * sizeof(struct triangle_vector));

    double xA, yA, zA, xB, yB, zB, xC, yC, zC, a, b, c, h, r;
    int i, j, A, B, C;
    for (i = 0; i < number_of_triangles; ++i)
    {
        A = triangles[i].vertex_a;
        B = triangles[i].vertex_b;
        C = triangles[i].vertex_c;

        if (A > number_of_vertices || A < 0 || B > number_of_vertices || B < 0 || C > number_of_vertices || C < 0)
        {
            continue;
        }

        if ((vertices + A) == NULL || (vertices + B) == NULL || (vertices + C) == NULL)
        {
            continue;
        }

        if (triangles[i].first == 1)
        {
            xA = (vertices + A)->x;
            yA = (vertices + A)->y;
            zA = (vertices + A)->z;
            xB = (vertices + B)->x;
            yB = (vertices + B)->y;
            zB = (vertices + B)->z;
            xC = (vertices + C)->x;
            yC = (vertices + C)->y;
            zC = (vertices + C)->z;

            a = yA * (zB - zC) - yB * (zA - zC) + yC * (zA - zB);
            b = -(xA * (zB - zC) - xB * (zA - zC) + xC * (zA - zB));
            c = xA * (yB - yC) - xB * (yA - yC) + xC * (yA - yB);
            h = xA * (yB * zC - yC * zB) - xB * (yA * zC - yC * zA) + xC * (yA * zB - yB * zA);

            r = sqrt(a * a + b * b + c * c);
            a /= r;
            b /= r;
            c /= r;
            h /= r;

            triangle_vectors[i].a = a;
            triangle_vectors[i].b = b;
            triangle_vectors[i].c = c;
            triangle_vectors[i].h = h;
        }
        else
        {
            j = i;
            while (triangles[j].first != 1)
            {
                --j;
            }
            triangle_vectors[i].a = triangle_vectors[j].a;
            triangle_vectors[i].b = triangle_vectors[j].b;
            triangle_vectors[i].c = triangle_vectors[j].c;
            triangle_vectors[i].h = triangle_vectors[j].h;
        }
    }

    return triangle_vectors;
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

struct vertice_2d project_vertex(struct vector_3d vertex)
{
    struct vertice_2d v;
    v.x = project_x_coordinate(vertex);
    v.y = project_y_coordinate(vertex);
    v.depth = vertex.z;
    return v;
}

int project_x_coordinate(struct vector_3d vertex)
{
    return (int)(vertex.x * d / vertex.z + c1);
}

int project_y_coordinate(struct vector_3d vertex)
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

struct vector_3d apply_rotation_matrix(struct obj_vertex vertice, struct obj_vertex original_vertice)
{
    double xO = original_vertice.x, yO = original_vertice.y, zO = original_vertice.z;
    double x = vertice.x;
    double y = vertice.y;
    double z = vertice.z;

    double pxe = (v11 * (vertice.x - original_vertice.x) + v21 * (y - yO)) + xO;
    double pye = (v12 * (x - xO) + v22 * (y - yO) + v32 * (z - zO)) + yO;
    double pze = (v13 * (x - xO) + v23 * (y - yO) + v33 * (z - zO) + v43) + zO;

    struct vector_3d rotated_vertice = {pxe, pye, pze};
    return rotated_vertice;
}

void calculate_rotation_matrix()
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
    for (int i = 1; i <= object->number_of_vertices; ++i)
    {
        object->vertices[i].x += dx;
        object->vertices[i].y += dy;
        object->vertices[i].z += dz;
    }
}

void move_obj_absolute(struct obj *object, int x, int y, int z)
{
    int dx = x - object->vertices[0].x;
    int dy = y - object->vertices[0].y;
    int dz = z - object->vertices[0].z;
    for (int i = 1; i <= object->number_of_vertices; ++i)
    {
        object->vertices[i].x += dx;
        object->vertices[i].y += dy;
        object->vertices[i].z += dz;
    }
}

void set_scale(float scale)
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
