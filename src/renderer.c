#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "../include/drawing.h"
#include "../include/renderer.h"
#include "../include/utils.h"

struct vector_3d light_direction = {0, 0, 255};

int x_viewpoint_range, y_viewpoint_range;

struct wavefront_obj current_object;

double *zbuffer;
struct vector_3d *vertices;
struct vector_3d *vertex_normals;
struct triangle_vector *triangle_vectors;

struct color current_color;
struct color current_edge_color;
int current_background_color;
double theta, phi, rho;
double alpha = 0, beta = 0, gamma = 0;
int dx, dy = 0;
double scale;
double initial_scale = 0;
int camera_distance = 500;
enum shading_type current_shading_type = FLAT;

const int BIG_INT = 2147483647;

void initialize_z_buffer();
void reset_z_buffer();

void render_triangles(struct wavefront_obj_triangle *triangles, struct triangle_vector *triangle_vectors, int number_of_triangles, struct vector_3d *vertices, int number_of_vertices);
void render_triangle(struct wavefront_obj_triangle triangle, struct triangle_vector triangle_vector, struct vector_3d *vertices, int number_of_vertices);
void rasterize_triangle(struct vertex_2d v1, struct vertex_2d v2, struct vertex_2d v3, struct vector_3d triangle_normal, struct color color);

double calculate_camera_distance(struct wavefront_obj object);
void calculate_rotated_vertices(struct wavefront_obj_vertex *object_vertices, int number_of_vertices);
void calculate_scale(struct vector_3d *vertices, int number_of_vertices);
void calculate_triangle_vectors(struct wavefront_obj_triangle *triangles, int number_of_triangles);
struct vertex_2d project_vertex(struct vector_3d vertex);
void normalize_vector(struct vector_3d *vec);
struct vector_3d rotate_vertex(struct wavefront_obj_vertex object_vertex);
struct triangle_vector calculate_triangle_vector(struct wavefront_obj_triangle triangle);
double calculate_vertex_intensity(int vertex_index);
void calculate_vertex_normals(struct wavefront_obj_triangle *triangles, int number_of_triangles, int number_of_vertices);

void initialize_renderer()
{
    x_viewpoint_range = x_viewpoint_max - x_viewpoint_min;
    y_viewpoint_range = y_viewpoint_max - y_viewpoint_min;
    initialize_z_buffer();

    if (current_object.number_of_vertices > 0)
    {
        calculate_rotated_vertices(current_object.vertices, current_object.number_of_vertices);
        calculate_scale(vertices, current_object.number_of_vertices);
    }
}

void set_object_to_render(struct wavefront_obj object)
{
    if (vertices != NULL)
    {
        free(vertices);
    }
    if (vertex_normals != NULL)
    {
        free(vertex_normals);
    }
    if (triangle_vectors != NULL)
    {
        free(triangle_vectors);
    }

    current_object = object;

    int number_of_vertices = current_object.number_of_vertices;
    int number_of_triangles = current_object.number_of_triangles;

    vertices = (struct vector_3d *)malloc(number_of_vertices * sizeof(struct vector_3d));
    vertex_normals = (struct vector_3d *)malloc(number_of_vertices * sizeof(struct vector_3d));
    triangle_vectors = (struct triangle_vector *)malloc(number_of_triangles * sizeof(struct triangle_vector));

    camera_distance = calculate_camera_distance(current_object);
    calculate_rotated_vertices(current_object.vertices, number_of_vertices);
    calculate_scale(vertices, number_of_vertices);
}

void render()
{
    fill_buffer(current_background_color);

    if (current_object.vertices == NULL || current_object.triangles == NULL)
    {
        return;
    }

    struct wavefront_obj_vertex *object_vertices = current_object.vertices;
    struct wavefront_obj_triangle *object_triangles = current_object.triangles;
    int number_of_vertices = current_object.number_of_vertices;
    int number_of_triangles = current_object.number_of_triangles;

    calculate_rotated_vertices(object_vertices, number_of_vertices);
    calculate_triangle_vectors(object_triangles, number_of_triangles);

    if (current_shading_type == GOURAUD)
    {
        calculate_vertex_normals(object_triangles, number_of_triangles, number_of_vertices);
    }

    render_triangles(object_triangles, triangle_vectors, number_of_triangles, vertices, number_of_vertices);
}

void render_triangles(struct wavefront_obj_triangle *triangles, struct triangle_vector *triangle_vectors, int number_of_triangles, struct vector_3d *vertices, int number_of_vertices)
{
    reset_z_buffer();
    normalize_vector(&light_direction);
    for (int i = 0; i < number_of_triangles; ++i)
    {
        render_triangle(triangles[i], triangle_vectors[i], vertices, number_of_vertices);
    }
}

void render_triangle(struct wavefront_obj_triangle triangle, struct triangle_vector triangle_vector, struct vector_3d *vertices, int number_of_vertices)
{
    double h = triangle_vector.h;

    if (h < 0)
    {
        // Negative h means triangle is facing the viewpoint
        return;
    }

    int vertex_a_idx = triangle.vertex_a;
    int vertex_b_idx = triangle.vertex_b;
    int vertex_c_idx = triangle.vertex_c;

    struct vector_3d vertex_a = vertices[vertex_a_idx];
    struct vector_3d vertex_b = vertices[vertex_b_idx];
    struct vector_3d vertex_c = vertices[vertex_c_idx];

    struct vertex_2d projected_vertex_a = project_vertex(vertex_a);
    struct vertex_2d projected_vertex_b = project_vertex(vertex_b);
    struct vertex_2d projected_vertex_c = project_vertex(vertex_c);

    if (current_shading_type == GOURAUD)
    {
        projected_vertex_a.intensity = calculate_vertex_intensity(vertex_a_idx);
        projected_vertex_b.intensity = calculate_vertex_intensity(vertex_b_idx);
        projected_vertex_c.intensity = calculate_vertex_intensity(vertex_c_idx);
    }

    rasterize_triangle(projected_vertex_a, projected_vertex_b, projected_vertex_c, triangle_vector.norm, current_color);
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

void calculate_rotated_vertices(struct wavefront_obj_vertex *object_vertices, int number_of_vertices)
{
    for (int i = 0; i < number_of_vertices; ++i)
    {
        vertices[i] = rotate_vertex(object_vertices[i]);
    }
}

void calculate_scale(struct vector_3d *vertices, int number_of_vertices)
{
    double x_viewpoint_center = (x_viewpoint_min + x_viewpoint_max) / 2;
    double y_viewpoint_center = (y_viewpoint_min + y_viewpoint_max) / 2;

    dx = x_viewpoint_center;
    dy = y_viewpoint_center;

    double x_min = BIG_INT, x_max = -BIG_INT, y_min = BIG_INT, y_max = -BIG_INT;

    for (int i = 0; i < number_of_vertices; i++)
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

    scale = fy > fx ? fx : fy;
    scale = scale * 0.8; // 80% of the screen
    initial_scale = scale;
}

void calculate_triangle_vectors(struct wavefront_obj_triangle *triangles, int number_of_triangles)
{
    // First we calculate vector of each triangle (which is a part of bigger polygon)
    for (int i = 0; i < number_of_triangles; ++i)
    {
        struct wavefront_obj_triangle triangle = triangles[i];
        if (triangles[i].part_of_polygon != -1)
        {
            continue;
        }

        triangle_vectors[i] = calculate_triangle_vector(triangle);
    }

    // Then we assign the same vector to all triangles that are part of the same polygon
    for (int i = 0; i < number_of_triangles; ++i)
    {
        struct wavefront_obj_triangle triangle = triangles[i];
        if (triangles[i].part_of_polygon == -1)
        {
            continue;
        }

        int polygon_index = triangles[i].part_of_polygon;
        triangle_vectors[i] = triangle_vectors[polygon_index];
    }
}

struct triangle_vector calculate_triangle_vector(struct wavefront_obj_triangle triangle)
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

// Read more: https://jtsorlinis.github.io/rendering-tutorial/
double edge_function(struct vertex_2d a, struct vertex_2d b, struct vertex_2d c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
};

void rasterize_triangle(struct vertex_2d v1, struct vertex_2d v2, struct vertex_2d v3, struct vector_3d triangle_normal, struct color base_color)
{
    if (v1.y == v2.y && v1.y == v3.y)
        return;

    double V2V1X = v2.x - v1.x;
    double V2V1Y = v2.y - v1.y;
    double V3V1X = v3.x - v1.x;
    double V3V1Y = v3.y - v1.y;
    double V3V2X = v3.x - v2.x;
    double V3V2Y = v3.y - v2.y;
    double V1V3X = v1.x - v3.x;
    double V1V3Y = v1.y - v3.y;

    float area = V2V1X * V3V1Y - V3V1X * V2V1Y; // edge_function(v1, v2, v3);
    if (area == 0)
        return; // Triangle has no area
    float area_inv = 1.0 / area;

    struct color triangle_color = base_color;

    if (current_shading_type == FLAT)
    {
        double intensity = light_direction.x * triangle_normal.x + light_direction.y * triangle_normal.y + light_direction.z * triangle_normal.z;
        if (intensity < 0)
        {
            intensity = 0;
        }

        triangle_color = (struct color){base_color.r * intensity, base_color.g * intensity, base_color.b * intensity};
    }

    // Compute bounding box
    int min_x = fmin(fmin(v1.x, v2.x), v3.x);
    int max_x = fmax(fmax(v1.x, v2.x), v3.x);
    int min_y = fmin(fmin(v1.y, v2.y), v3.y);
    int max_y = fmax(fmax(v1.y, v2.y), v3.y);

    // Clamp bounding box to viewport
    min_y = fmax(min_y, y_viewpoint_min + 1);
    max_y = fmin(max_y, y_viewpoint_max - 1);

    min_x = fmax(min_x, x_viewpoint_min + 1);
    max_x = fmin(max_x, x_viewpoint_max - 1);

    // Loop over bounding box
    for (int y = min_y; y <= max_y; y++)
    {
        // Precompute edge functions
        double V3V2X_VPV2Y = V3V2X * (y - v2.y);
        double V1V3X_VPV3Y = V1V3X * (y - v3.y);
        double V2V1X_VPV1Y = V2V1X * (y - v1.y);

        for (int x = min_x; x <= max_x; x++)
        {
            // Compute barycentric coordinates
            float w1 = (V3V2X_VPV2Y - V3V2Y * (x - v2.x)) * area_inv; // edge_function(v2, v3, p {x, y})
            float w2 = (V1V3X_VPV3Y - V1V3Y * (x - v3.x)) * area_inv; // edge_function(v3, v1, p {x, y})
            float w3 = (V2V1X_VPV1Y - V2V1Y * (x - v1.x)) * area_inv; // edge_function(v1, v2, p {x, y})

            if (w1 < 0 || w2 < 0 || w3 < 0)
                // If outside triangle
                continue;

            // Interpolate depth (z)
            double z = w1 * v1.depth + w2 * v2.depth + w3 * v3.depth;
            if (z <= 0)
                continue; // Behind the viewpoint

            int pixel_index = x + x_viewpoint_range * y;
            if (zbuffer[pixel_index] <= z)
                continue; // Pixel is behind the drawn one

            if (current_shading_type == NO_SHADING)
            {
                if (w1 <= 0.01 || w2 <= 0.01 || w3 <= 0.01)
                    draw_pixel(x, y, current_edge_color.r, current_edge_color.g, current_edge_color.b); // Pixel is on the edge of the triangle
                else
                    draw_pixel(x, y, triangle_color.r, triangle_color.g, triangle_color.b);
            }
            else if (current_shading_type == FLAT)
            {
                draw_pixel(x, y, triangle_color.r, triangle_color.g, triangle_color.b);
            }
            else if (current_shading_type == GOURAUD)
            {
                double intensity = w1 * v1.intensity + w2 * v2.intensity + w3 * v3.intensity;
                draw_pixel(x, y, base_color.r * intensity, base_color.g * intensity, base_color.b * intensity);
            }
            zbuffer[pixel_index] = z;
        }
    }
}

struct vertex_2d project_vertex(struct vector_3d vertex)
{
    struct vertex_2d v;
    v.x = (int)((vertex.x / vertex.z) * scale + dx);
    v.y = (int)((vertex.y / vertex.z) * scale + dy);
    v.depth = vertex.z;
    return v;
}

void normalize_vector(struct vector_3d *vec)
{
    double r;
    r = sqrt(SQR(vec->x) + SQR(vec->y) + SQR(vec->z));
    vec->x /= r;
    vec->y /= r;
    vec->z /= r;
}

struct vector_3d rotate_vertex(struct wavefront_obj_vertex object_vertex)
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

    z_rot += camera_distance;

    // Translate back to original position
    struct vector_3d rotated_vertex = {x_rot, y_rot, z_rot};
    return rotated_vertex;
}

void reset_z_buffer()
{
    int buffer_length = (x_viewpoint_range + 1) * (y_viewpoint_range + 1);
    for (int i = 0; i < buffer_length; i++)
    {
        *(zbuffer + i) = BIG_INT;
    }
}

double calculate_vertex_intensity(int vertex_index)
{
    struct vector_3d vertex_normal = vertex_normals[vertex_index];
    double intensity = light_direction.x * vertex_normal.x +
                       light_direction.y * vertex_normal.y +
                       light_direction.z * vertex_normal.z;

    if (intensity < 0)
        intensity = 0;

    return intensity;
}

void calculate_vertex_normals(struct wavefront_obj_triangle *triangles, int number_of_triangles, int number_of_vertices)
{
    // Initialize all vertex normals to (0, 0, 0)
    for (int i = 0; i < number_of_vertices; ++i)
    {
        vertex_normals[i].x = 0;
        vertex_normals[i].y = 0;
        vertex_normals[i].z = 0;
    }

    // Calculate and add triangle normals to vertex normals
    for (int i = 0; i < number_of_triangles; ++i)
    {
        struct wavefront_obj_triangle triangle = triangles[i];
        struct vector_3d triangle_normal = triangle_vectors[i].norm;

        int vertex_a_idx = triangle.vertex_a;
        int vertex_b_idx = triangle.vertex_b;
        int vertex_c_idx = triangle.vertex_c;

        // Add the triangle normal to the normals of each vertex
        vertex_normals[vertex_a_idx].x += triangle_normal.x;
        vertex_normals[vertex_a_idx].y += triangle_normal.y;
        vertex_normals[vertex_a_idx].z += triangle_normal.z;

        vertex_normals[vertex_b_idx].x += triangle_normal.x;
        vertex_normals[vertex_b_idx].y += triangle_normal.y;
        vertex_normals[vertex_b_idx].z += triangle_normal.z;

        vertex_normals[vertex_c_idx].x += triangle_normal.x;
        vertex_normals[vertex_c_idx].y += triangle_normal.y;
        vertex_normals[vertex_c_idx].z += triangle_normal.z;
    }

    // Normalize all vertex normals
    for (int i = 0; i < number_of_vertices; ++i)
    {
        normalize_vector(&vertex_normals[i]);
    }
}

double calculate_camera_distance(struct wavefront_obj object)
{
    double fov = 30;
    double fov_radians = DEG_TO_RAD(fov);
    double largest_dimension = sqrt(SQR(object.x_max - object.x_min) + SQR(object.y_max - object.y_min) + SQR(object.z_max - object.z_min));
    return largest_dimension / (2 * tan(fov_radians / 2));
}

void set_scale(float scale_multiplier)
{
    scale = initial_scale * scale_multiplier;
}

int get_camera_distance()
{
    return camera_distance;
}

void set_colors(int background_color, struct color color, struct color edge_color)
{
    current_color = color;
    current_edge_color = edge_color;
    current_background_color = background_color;
}

void move_obj_relative(int dx_value, int dy_value)
{
    dx += dx_value;
    dy += dy_value;
}

void set_shading_type(enum shading_type shading_type)
{
    current_shading_type = shading_type;
}

void set_cartesian_translation(double alpha_value, double beta_value, double gamma_value)
{
    alpha = alpha_value;
    beta = beta_value;
    gamma = gamma_value;
}

void set_camera_distance(int value)
{
    camera_distance = value;
}