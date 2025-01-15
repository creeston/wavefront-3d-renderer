struct vertice
{
    double x, y, z;
    int *connect;
    double a, b, c;
};

struct triangle
{
    int vertex_a, vertex_b, vertex_c;
    double a, b, c, h;
    int first;
};

struct obj
{
    struct vertice *fvertex;
    struct vertice *vertex;
    int nvr;
    struct triangle *triangle;
    int ntr;
    int n;
};

#define BIG 1e30

void read_object(char *filename, struct obj *object);
