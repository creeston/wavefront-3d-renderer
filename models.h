#ifndef MODELS_INCLUDE_H
#define MODELS_INCLUDE_H

struct wavefront_obj_vertex
{
    double x, y, z;
};

struct wavefront_obj_triangle
{
    int vertex_a, vertex_b, vertex_c;
    int part_of_polygon;
};

struct wavefront_obj
{
    struct wavefront_obj_vertex *vertices;
    struct wavefront_obj_triangle *triangles;
    int number_of_triangles;
    int number_of_vertices;
    double x_min, x_max, y_min, y_max, z_min, z_max;
};

enum shading_type
{
    NO_SHADING,
    FLAT,
    GOURAUD,
};

struct color
{
    int r, g, b;
};

struct vertex_2d
{
    int x, y;
    float depth;
    double intensity;
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

#endif
