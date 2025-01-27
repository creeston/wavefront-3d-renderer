#include "renderer.h"
#include "utils.h"
#include <string.h>
#include <assert.h>

struct vertex_2d
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

int c1, c2;
int x_viewpoint_range, y_viewpoint_range;

double *zbuffer;
struct obj *previsouly_rendered_object;
struct obj *current_object;
struct vector_3d *vertices;
struct triangle_vector *triangle_vectors;
struct color current_color;

double theta, phi, rho;
int translation = 500;
double alpha = 0, beta = 0, gamma = 0;
int dx, dy = 0;

int pixel_x_info = -1, pixel_y_info = -1;

#define BIG_INT 2147483647
#define EPS 1.e-5
#define M_EPS -1.e-5
#define ONE_PLUS 1 + 1.e-5
#define ONE_MINUS 1 - 1.e-5

#define M_PI 3.14159265358979323846

#define SQR(x) ((x) * (x))
#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)
#define RAD_TO_DEG(rad) ((rad) * 180.0 / M_PI)

void rasterize_triangle(struct vertex_2d A, struct vertex_2d B, struct vertex_2d C, struct color color);
void render_triangles(struct obj_triangle *triangles, struct triangle_vector *triangle_vectors, int number_of_triangles, struct vector_3d *vertices, int number_of_vertices);
void render_triangle(struct obj_triangle triangle, struct triangle_vector triangle_vector, struct vector_3d *vertices, int number_of_vertices);
void rasterize_line(int x_i, double z_i, int x_j, double z_j, int y, struct color color);
void rasterize_vertex(struct vertex_2d vertex, struct color color);
void initialize_z_buffer();
void calculate_rotated_vertices(struct obj_vertex *object_vertices, int number_of_vertices);
void calculate_scale(struct vector_3d *vertices, int number_of_vertices);
void calculate_triangle_vectors(struct obj_triangle *triangles, int number_of_triangles);
int project_x_coordinate(struct vector_3d vertex);
int project_y_coordinate(struct vector_3d vertex);
struct vertex_2d project_vertex(struct vector_3d vertex);
void normalize_vector(struct vector_3d *vec);
struct vector_3d rotate_vertex(struct obj_vertex object_vertex);
void reset_z_buffer();
void swap_vertices(struct vertex_2d *A, struct vertex_2d *B);
void calculate_rotation_matrix();
struct triangle_vector calculate_triangle_vector(struct obj_triangle triangle);

void initialize_renderer()
{
    x_viewpoint_range = x_viewpoint_max - x_viewpoint_min;
    y_viewpoint_range = y_viewpoint_max - y_viewpoint_min;
    initialize_z_buffer();
}

void get_pixel_info(int x, int y)
{
    pixel_x_info = x;
    pixel_y_info = y;
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

    double fov = 30;
    double fov_radians = DEG_TO_RAD(fov);
    translation = object->largest_dimension / (2 * tan(fov_radians / 2));

    calculate_rotated_vertices(object_vertices, number_of_vertices);
    calculate_scale(vertices, number_of_vertices);
}

int get_translation()
{
    return translation;
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
    calculate_triangle_vectors(object_triangles, number_of_triangles);
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
    struct vertex_2d vertex_a = project_vertex(vertices[vertex_a_idx]);
    struct vertex_2d vertex_b = project_vertex(vertices[vertex_b_idx]);
    struct vertex_2d vertex_c = project_vertex(vertices[vertex_c_idx]);

    rasterize_triangle(vertex_a, vertex_b, vertex_c, intensity_modified_color);
}

void initialize_z_buffer()
{
    if (zbuffer != NULL)
    {
        free(zbuffer);
    }

    int buffer_length = (x_viewpoint_range + 1) * (y_viewpoint_range + 1);
    zbuffer = (double *)malloc(buffer_length * sizeof(double));
    for (int i = 0; i < buffer_length; i++)
    {
        *(zbuffer + i) = BIG_INT;
    }
}

void calculate_rotated_vertices(struct obj_vertex *object_vertices, int number_of_vertices)
{
    for (int i = 1; i <= number_of_vertices; ++i)
    {
        struct obj_vertex object_vertex = object_vertices[i];
        vertices[i] = rotate_vertex(object_vertex);
    }
}

void calculate_scale(struct vector_3d *vertices, int number_of_vertices)
{
    double x_viewpoint_center = (x_viewpoint_min + x_viewpoint_max) / 2;
    double y_viewpoint_center = (y_viewpoint_min + y_viewpoint_max) / 2;

    dx = x_viewpoint_center;
    dy = y_viewpoint_center;

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

    d = fy;
    initial_d = d;
}

void calculate_triangle_vectors(struct obj_triangle *triangles, int number_of_triangles)
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

void rasterize_triangle(struct vertex_2d v1, struct vertex_2d v2, struct vertex_2d v3, struct color color)
{
    // Uncomment the following lines to draw the vertices of the triangle
    // rasterize_vertex(v1, color);
    // rasterize_vertex(v2, color);
    // rasterize_vertex(v3, color);
    // return;

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

    int total_height = v3.y - v1.y; // v1 is the bottom vertex, v3 is the top vertex
    int first_segment_height = v2.y - v1.y;
    int second_segment_height = v3.y - v2.y;

    if (total_height == 0)
    {
        return;
    }

    int y_start = v1.y;

    for (int y_index = 0; y_index < first_segment_height; y_index++)
    {
        int y = y_start + y_index;
        if (y <= y_viewpoint_min || y >= y_viewpoint_max)
        {
            // Do not draw pixels outside the viewport
            continue;
        }

        double alpha = (double)(y_index) / total_height;
        double beta = (double)(y_index) / first_segment_height;

        int x_start = alpha * v3.x + v1.x * (1 - alpha);
        double z_start = alpha * v3.depth + v1.depth * (1 - alpha);

        int x_end = beta * v2.x + v1.x * (1 - beta);
        double z_end = beta * v2.depth + v1.depth * (1 - beta);

        if (x_start > x_end)
        {
            swap_integers(&x_start, &x_end);
        }

        rasterize_line(x_start, z_start, x_end, z_end, y, color);
    }

    if (second_segment_height == 0)
    {
        return;
    }

    for (int y_index = first_segment_height; y_index < total_height; y_index++)
    {
        int y = y_start + y_index;
        if (y <= y_viewpoint_min || y >= y_viewpoint_max)
        {
            continue;
        }

        double alpha = (float)(y_index) / total_height;
        double beta = (float)(y_index - first_segment_height) / second_segment_height;

        int x_start = alpha * v3.x + v1.x * (1 - alpha);
        double z_start = alpha * v3.depth + v1.depth * (1 - alpha);

        int x_end = beta * v3.x + v2.x * (1 - beta);
        double z_end = beta * v3.depth + v2.depth * (1 - beta);

        if (x_start > x_end)
        {
            swap_integers(&x_start, &x_end);
        }

        rasterize_line(x_start, z_start, x_end, z_end, y, color);
    }
}

void rasterize_vertex(struct vertex_2d vertex, struct color color)
{
    int x = vertex.x;
    int y = vertex.y;

    if (x <= x_viewpoint_min || x >= x_viewpoint_max || y <= y_viewpoint_min || y >= y_viewpoint_max)
    {
        return;
    }

    int pixel_index = x + x_viewpoint_range * y;
    double current_z_value = zbuffer[pixel_index];
    if (current_z_value <= vertex.depth)
    {
        return;
    }

    draw_pixel(x, y, current_color);
    zbuffer[pixel_index] = vertex.depth;
}

void rasterize_line(int x_start, double z_start, int x_end, double z_end, int y, struct color color)
{
    int line_length = x_end - x_start;
    double line_depth = z_end - z_start;
    for (int x_index = 0; x_index <= line_length; x_index++)
    {
        int x = x_start + x_index;
        if (x <= x_viewpoint_min || x >= x_viewpoint_max)
        {
            // Do not draw pixels outside the viewport
            continue;
        }

        double phi = line_length == 0 ? 1. : (double)x_index / (double)line_length; // ranges from 0 to 1 depending on current x
        double z = z_start + line_depth * phi;                                      // interpolate z value
        if (z <= 0)
        {
            // Do not draw pixels behind the viewpoint
            continue;
        }

        if (x == pixel_x_info && y == pixel_y_info)
        {
            printf("X: %d, Y: %d, Z value: %f\n", x, y, z);
            pixel_x_info = -1;
            pixel_y_info = -1;
        }

        int pixel_index = x + x_viewpoint_range * y;
        double current_z_value = zbuffer[pixel_index];
        if (current_z_value <= z)
        {
            // then pixel is behind the current one
            continue;
        }

        draw_pixel(x, y, color);
        zbuffer[pixel_index] = z;
    }
}

struct vertex_2d project_vertex(struct vector_3d vertex)
{
    struct vertex_2d v;
    v.x = project_x_coordinate(vertex);
    v.y = project_y_coordinate(vertex);
    v.depth = vertex.z;
    return v;
}

int project_x_coordinate(struct vector_3d vertex)
{
    return (int)((vertex.x / vertex.z) * d + dx);
}

int project_y_coordinate(struct vector_3d vertex)
{
    return (int)((vertex.y / vertex.z) * d + dy);
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

struct vector_3d rotate_vertex(struct obj_vertex object_vertex)
{
    double x = object_vertex.x;
    double y = object_vertex.y;
    double z = object_vertex.z;

    // Precompute trigonometric values
    double cos_alpha = cos(alpha), sin_alpha = sin(alpha);
    double cos_beta = cos(beta), sin_beta = sin(beta);
    double cos_gamma = cos(gamma), sin_gamma = sin(gamma);

    // Combined rotation matrix
    double r11 = cos_beta * cos_gamma;
    double r12 = cos_beta * sin_gamma;
    double r13 = -sin_beta;

    double r21 = sin_alpha * sin_beta * cos_gamma - cos_alpha * sin_gamma;
    double r22 = sin_alpha * sin_beta * sin_gamma + cos_alpha * cos_gamma;
    double r23 = sin_alpha * cos_beta;

    double r31 = cos_alpha * sin_beta * cos_gamma + sin_alpha * sin_gamma;
    double r32 = cos_alpha * sin_beta * sin_gamma - sin_alpha * cos_gamma;
    double r33 = cos_alpha * cos_beta;

    // Apply rotation matrix
    double x_rot = r11 * x + r12 * y + r13 * z;
    double y_rot = r21 * x + r22 * y + r23 * z;
    double z_rot = r31 * x + r32 * y + r33 * z;

    z_rot += translation;

    // Translate back to original position
    struct vector_3d rotated_vertex = {x_rot, y_rot, z_rot};
    return rotated_vertex;
}

void set_cartesian_translation(double alpha_value, double beta_value, double gamma_value)
{
    alpha = alpha_value;
    beta = beta_value;
    gamma = gamma_value;
}

void set_translation(int translation_value)
{
    translation = translation_value;
}

void calculate_rotation_matrix()
{
    double costh, sinth, cosph, sinph, factor;

    costh = cos(theta);
    sinth = sin(theta);
    cosph = cos(phi);
    sinph = sin(phi);

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

void move_obj_relative(int dx_value, int dy_value)
{
    dx += dx_value;
    dy += dy_value;
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

void swap_vertices(struct vertex_2d *A, struct vertex_2d *B)
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
