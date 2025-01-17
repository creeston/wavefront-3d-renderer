struct obj_vertex
{
    double x, y, z;
    int *connect;
    double a, b, c;
};

struct obj_triangle
{
    int vertex_a, vertex_b, vertex_c;
    int first;
};

struct obj
{
    struct obj_vertex *vertices;
    struct obj_triangle *triangles;
    int number_of_triangles;
    int number_of_vertices;
};

#define BIG 1e30

void read_object(char *filename, struct obj *object);
