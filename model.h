struct vertice
{
    double x, y, z;
    int *connect;
    double a, b, c;
};

struct triangle
{
    int A, B, C;
    double a, b, c, h;
    int first;
};

struct obj
{
    struct vertice *fvertex, *vertex;
    int nvr;
    struct triangle *triangle;
    int ntr;
    int n;
};

#define BIG 1e30

void read_object(char *filename, struct obj *object);
