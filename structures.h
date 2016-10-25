struct ver {
    double x,y,z;
    int *connect;
    double a, b ,c;
};

struct tr{
    int A,B,C;
    double a,b,c,h;
    int first;
};

struct obj {
    struct ver *fvertex, *vertex;
    int nvr;
    struct tr* triangle;
    int ntr;
    int n;
};

struct color{
    int r,g,b;
};

struct v2D{
    int x,y;
    float depth;
};

struct v3D {
    double x,y,z;
};