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
    // normalized normal vector to the plane.
    struct vector_3d norm;

    // Distance from the triangle plane to the origin along the triangle's normal direction
    // The signed value of h distinguishes the side of the plane the origin lies on:
    // Positive h means the origin is on one side of the plane.
    // Negative h means the origin is on the opposite side.
    double h;
};

struct vector_3d light_direction = {0, 0, 255};
double v11, v12, v13, v21, v22, v23, v32, v33, v43;
double d;
double initial_d = 0;
double c1, c2;
double x_viewpoint_range, y_viewpoint_range;

int *zbuffer;
struct obj *previsouly_rendered_object;
struct obj *current_object;
struct vector_3d *vertices;
struct triangle_vector *triangle_vectors;
struct color current_color;

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
void render_triangles(struct obj_triangle *triangles, struct triangle_vector *triangle_vectors, int number_of_triangles, struct vector_3d *vertices, int number_of_vertices);
void render_triangle(struct obj_triangle triangle, struct triangle_vector triangle_vector, struct vector_3d *vertices, int number_of_vertices);
void initialize_z_buffer();
void calculate_rotated_vertices(struct obj_vertex *object_vertices, int number_of_vertices);
void calculate_scale(struct vector_3d *vertices, int number_of_vertices);
void calculate_triangle_vectors(struct vector_3d *vertex, struct obj_triangle *triangle, int ntr, int nvr);
int project_x_coordinate(struct vector_3d vertex);
int project_y_coordinate(struct vector_3d vertex);
struct vertice_2d project_vertex(struct vector_3d vertex);
void normalize_vector(struct vector_3d *vec);
struct vector_3d apply_rotation_matrix(struct obj_vertex vertice, struct obj_vertex original_vertice);
void reset_z_buffer();
void swap_vertices(struct vertice_2d *A, struct vertice_2d *B);
void calculate_rotation_matrix();
struct triangle_vector calculate_triangle_vector(struct obj_triangle triangle);

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

void set_scale(float scale)
{
    d = initial_d * scale;
}

void set_object_to_render(struct obj *object)
{
    if (current_object != NULL)
    {
        free(vertices);
        free(triangle_vectors);
    }

    current_object = object;

    struct obj_vertex *object_vertices = object->vertices;
    int number_of_vertices = object->number_of_vertices;
    int number_of_triangles = object->number_of_triangles;

    vertices = (struct vector_3d *)malloc((number_of_vertices + 1) * sizeof(struct vector_3d));
    triangle_vectors = (struct triangle_vector *)malloc(number_of_triangles * sizeof(struct triangle_vector));

    calculate_rotated_vertices(object_vertices, number_of_vertices);
    calculate_scale(vertices, number_of_vertices);
}

void set_color(struct color color)
{
    current_color = color;
}

void render()
{
    if (current_object == NULL)
    {
        return;
    }

    struct obj_vertex *object_vertices = current_object->vertices;
    struct obj_triangle *object_triangles = current_object->triangles;
    int number_of_vertices = current_object->number_of_vertices;
    int number_of_triangles = current_object->number_of_triangles;

    calculate_rotated_vertices(object_vertices, number_of_vertices);
    calculate_triangle_vectors(vertices, object_triangles, number_of_triangles, number_of_vertices);
    render_triangles(object_triangles, triangle_vectors, number_of_triangles, vertices, number_of_vertices);
}

void render_triangles(struct obj_triangle *triangles, struct triangle_vector *triangle_vectors, int number_of_triangles, struct vector_3d *vertices, int number_of_vertices)
{
    reset_z_buffer();
    normalize_vector(&light_direction);
    for (int i = 0; i < number_of_triangles; ++i)
    {
        render_triangle(triangles[i], triangle_vectors[i], vertices, number_of_vertices);
    }
}

void render_triangle(struct obj_triangle triangle, struct triangle_vector triangle_vector, struct vector_3d *vertices, int number_of_vertices)
{
    double h = triangle_vector.h;

    if (h < 0)
    {
        // Negative h means the origin is on the opposite side.
        return;
    }

    struct vector_3d norm = triangle_vector.norm;
    double intensity = light_direction.x * norm.x + light_direction.y * norm.y + light_direction.z * norm.z;
    if (intensity < 0)
    {
        intensity = 0;
    }

    int vertex_a_idx = triangle.vertex_a;
    int vertex_b_idx = triangle.vertex_b;
    int vertex_c_idx = triangle.vertex_c;

    struct color intensity_modified_color = {intensity * current_color.r, intensity * current_color.g, intensity * current_color.b};
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

void calculate_rotated_vertices(struct obj_vertex *object_vertices, int number_of_vertices)
{
    struct obj_vertex zero_vertex = object_vertices[0];
    for (int i = 1; i <= number_of_vertices; ++i)
    {
        struct obj_vertex object_vertex = object_vertices[i];
        vertices[i] = apply_rotation_matrix(object_vertex, zero_vertex);
    }
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

void calculate_triangle_vectors(struct vector_3d *vertices, struct obj_triangle *triangles, int number_of_triangles, int number_of_vertices)
{
    // First we calculate vector of each triangle (which is a part of bigger polygon)
    for (int i = 0; i < number_of_triangles; ++i)
    {
        struct obj_triangle triangle = triangles[i];
        if (triangles[i].part_of_polygon != -1)
        {
            continue;
        }

        triangle_vectors[i] = calculate_triangle_vector(triangle);
    }

    // Then we assign the same vector to all triangles that are part of the same polygon
    for (int i = 0; i < number_of_triangles; ++i)
    {
        struct obj_triangle triangle = triangles[i];
        if (triangles[i].part_of_polygon == -1)
        {
            continue;
        }

        int polygon_index = triangles[i].part_of_polygon;
        triangle_vectors[i] = triangle_vectors[polygon_index];
    }
}

struct triangle_vector calculate_triangle_vector(struct obj_triangle triangle)
{
    int vertex_a_index = triangle.vertex_a;
    int vertex_b_index = triangle.vertex_b;
    int vertex_c_index = triangle.vertex_c;

    struct vector_3d vertex_a = vertices[vertex_a_index];
    struct vector_3d vertex_b = vertices[vertex_b_index];
    struct vector_3d vertex_c = vertices[vertex_c_index];

    double x = vertex_a.y * (vertex_b.z - vertex_c.z) - vertex_b.y * (vertex_a.z - vertex_c.z) + vertex_c.y * (vertex_a.z - vertex_b.z);
    double y = -(vertex_a.x * (vertex_b.z - vertex_c.z) - vertex_b.x * (vertex_a.z - vertex_c.z) + vertex_c.x * (vertex_a.z - vertex_b.z));
    double z = vertex_a.x * (vertex_b.y - vertex_c.y) - vertex_b.x * (vertex_a.y - vertex_c.y) + vertex_c.x * (vertex_a.y - vertex_b.y);
    double h = vertex_a.x * (vertex_b.y * vertex_c.z - vertex_c.y * vertex_b.z) - vertex_b.x * (vertex_a.y * vertex_c.z - vertex_c.y * vertex_a.z) + vertex_c.x * (vertex_a.y * vertex_b.z - vertex_b.y * vertex_a.z);

    double r = sqrt(x * x + y * y + z * z);
    x /= r;
    y /= r;
    z /= r;
    h /= r;

    struct triangle_vector triangle_vector = {x, y, z, h};
    return triangle_vector;
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

        double x_i = alpha * v3.x + v1.x * (1 - alpha);
        double y_i = alpha * v3.y + v1.y * (1 - alpha);
        double z_i = alpha * v3.depth + v1.depth * (1 - alpha);

        double x_j = second_half ? (beta * v3.x + v2.x * (1 - beta)) : beta * v2.x + v1.x * (1 - beta);
        double y_j = second_half ? (beta * v3.y + v2.y * (1 - beta)) : beta * v2.y + v1.y * (1 - beta);
        double z_j = second_half ? (beta * v3.depth + v2.depth * (1 - beta)) : beta * v2.depth + v1.depth * (1 - beta);

        if (x_i > x_j)
        {
            swap_doubles(&x_i, &x_j);
            swap_doubles(&y_i, &y_j);
        }

        int y = v1.y + i;
        for (int x = x_i; x <= x_j; ++x)
        {
            if (x <= x_viewpoint_min || x >= x_viewpoint_max || y <= y_viewpoint_min || y >= y_viewpoint_max)
            {
                // Do not draw pixels outside the viewport
                continue;
            }

            double phi = x_j == x_i ? 1. : (float)(x - x_i) / (float)(x_j - x_i); // ranges from 0 to 1 depending on current x
            int pixel_z_value = z_i + (z_j - z_i) * phi;                          // interpolate z value
            int pixel_index = x + x_viewpoint_range * y;
            int current_z_value = zbuffer[pixel_index];
            if (current_z_value <= pixel_z_value)
            {
                // then pixel is behind the current one
                continue;
            }

            draw_pixel(x, y, color);
            zbuffer[pixel_index] = pixel_z_value;
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

struct vector_3d apply_rotation_matrix(struct obj_vertex object_vertex, struct obj_vertex original_vertex)
{
    double x_o = original_vertex.x;
    double y_o = original_vertex.y;
    double z_o = original_vertex.z;

    double x = object_vertex.x;
    double y = object_vertex.y;
    double z = object_vertex.z;

    double pxe = (v11 * (x - x_o) + v21 * (y - y_o)) + x_o;
    double pye = (v12 * (x - x_o) + v22 * (y - y_o) + v32 * (z - z_o)) + y_o;
    double pze = (v13 * (x - x_o) + v23 * (y - y_o) + v33 * (z - z_o) + v43) + z_o;

    struct vector_3d rotated_vertex = {pxe, pye, pze};
    return rotated_vertex;
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
