#include "renderer.h"
#include "utils.h"
#include <string.h>

#define sqr(x) ((x) * (x))

struct v2D
{
    int x, y;
    float depth;
};

struct v3D
{
    double x, y, z;
};

struct v3D lightdir = {0, 0, 1};
double v11, v12, v13, v21, v22, v23, v32, v33, v43, d, c1, c2, onetime = 1, Xvp_range, Yvp_range;
double initial_d = 0;
double eps = 1.e-5, meps = -1.e-5, oneminus = 1 - 1.e-5, oneplus = 1 + 1.e-5;
int *zbuffer;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)
#define RAD_TO_DEG(rad) ((rad) * 180.0 / M_PI)

void rast_triangles(struct v2D A, struct v2D B, struct v2D C, struct color Cl);
void draw_triangle(struct triangle *triangle, int ntr, struct vertice *vertex, struct color Cl);
void init_z_buffer();
void calculate_vertexes(struct vertice *fvertex, struct vertice **vertex, int nvr, double *Range);
void calculate_scale(double Xmin, double Xmax, double Ymin, double Ymax);
void calculate_triangles(struct vertice *vertex, struct triangle **triangle, int ntr, int nvr);
int proect_coordinate(char flag, struct vertice *vertex, int num);
void normalize_vector(struct v3D *vec);
void viewing(double x, double xO, double y, double yO, double z, double zO, double *pxe, double *pye, double *pze);
void count_coefficients(double rho, double theta, double phi);
void update_z_buffer();

void swapS(struct v2D *A, struct v2D *B)
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

void draw_obj(struct obj *object, struct color C, double theta, double phi, double rho)
{
    update_z_buffer();
    struct vertice **fvertex = &(object->fvertex);
    struct vertice **vertex = &(object->vertex);
    int nvr = object->nvr;

    struct triangle **triangle = &object->triangle;
    int ntr = object->ntr;

    double Range[4] = {BIG, -BIG, BIG, -BIG};
    count_coefficients(rho, theta, phi);

    calculate_vertexes(*fvertex, vertex, nvr, Range);

    if (onetime)
    {
        calculate_scale(Range[0], Range[1], Range[2], Range[3]);
        init_z_buffer();
    }
    onetime = 0;

    calculate_triangles(*vertex, triangle, ntr, nvr);
    draw_triangle(*triangle, ntr, *vertex, C);
}

void draw_triangle(struct triangle *triangle, int ntr, struct vertice *vertex, struct color Cl)
{
    int i, XA, YA, XB, YB, XC, YC, A, B, C;
    double a, b, c, h, intensity;
    normalize_vector(&lightdir);
    for (i = 0; i < ntr; ++i)
    {
        a = triangle[i].a;
        b = triangle[i].b;
        c = triangle[i].c;
        h = triangle[i].h;
        A = triangle[i].A;
        B = triangle[i].B;
        C = triangle[i].C;

        intensity = lightdir.x * a + lightdir.y * b + lightdir.z * c;
        if (h > 0 && intensity > 0)
        {
            struct color rgb = {intensity * Cl.r, intensity * Cl.g, intensity * Cl.b};

            if (A > ntr || B > ntr || C > ntr)
                continue;
            XA = proect_coordinate('x', vertex, A);
            YA = proect_coordinate('y', vertex, A);
            XB = proect_coordinate('x', vertex, B);
            YB = proect_coordinate('y', vertex, B);
            XC = proect_coordinate('x', vertex, C);
            YC = proect_coordinate('y', vertex, C);

            struct v2D vA = {XA, YA, vertex[A].z};
            struct v2D vB = {XB, YB, vertex[B].z};
            struct v2D vC = {XC, YC, vertex[C].z};

            rast_triangles(vA, vB, vC, rgb);
        }
    }
}

void init_z_buffer()
{
    int i;
    zbuffer = (int *)malloc((Xvp_range + 1) * (Yvp_range + 1) * sizeof(int));
    for (i = 0; i < (Xvp_range + 1) * (Yvp_range + 1); ++i)
        *(zbuffer + i) = 20000;
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
        if (ze <= eps)
            continue;

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
            printf("some kind of error\n");
    }
}

void calculate_scale(double Xmin, double Xmax, double Ymin, double Ymax)
{
    double fx, fy, Xcentre, Ycentre, Xvp_centre, Yvp_centre;
    double Xrange, Yrange;

    Xrange = Xmax - Xmin;
    Yrange = Ymax - Ymin;
    Xvp_range = x_viewpoint_max - x_viewpoint_min;
    Yvp_range = y_viewpoint_max - y_viewpoint_min;
    fx = Xvp_range / Xrange;
    fy = Yvp_range / Yrange;
    d = (fx < fy) ? fx : fy;
    initial_d = d;
    Xcentre = (Xmin + Xmax) / 2;
    Ycentre = (Ymax + Ymin) / 2;
    Xvp_centre = (x_viewpoint_min + x_viewpoint_max) / 2;
    Yvp_centre = (y_viewpoint_min + y_viewpoint_max) / 2;
    c1 = Xvp_centre - d * Xcentre;
    c2 = Yvp_centre - d * Ycentre;
}

void calculate_triangles(struct vertice *vertex, struct triangle **triangle, int ntr, int nvr)
{
    double xA, yA, zA, xB, yB, zB, xC, yC, zC, a, b, c, h, r;
    int i, j, A, B, C;
    for (i = 0; i < ntr; ++i)
    {
        A = (*triangle + i)->A;
        B = (*triangle + i)->B;
        C = (*triangle + i)->C;
        /*После выполнения функции RotateObj / MoveObj обьект, видимо, немного повреждается
        и некоторые значения номеров треугольников очень сильно меняются
        поэтому необходима проверка их соотвествия действительности, иначе - SegFault*/
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

void rast_triangles(struct v2D A, struct v2D B, struct v2D C, struct color Cl)
{
    if (A.y == B.y && A.y == C.y)
    {
        return;
    }
    if (A.y > B.y)
    {
        swapS(&A, &B);
    }
    if (A.y > C.y)
    {
        swapS(&A, &C);
    }
    if (B.y > C.y)
    {
        swapS(&B, &C);
    }

    int total_height = C.y - A.y, i, second_half, j, idx, segment_height;
    float alpha, beta, phi;
    double xI, yI, zI, xJ, yJ, zJ, Pz;
    long double time_spent_on_calculating = 0, time_spent_on_drawing = 0;
    for (i = 0; i <= total_height; ++i)
    {
        second_half = i > B.y - A.y || B.y == A.y;
        segment_height = second_half ? C.y - B.y : B.y - A.y;

        alpha = (float)(i) / total_height;
        beta = (float)(i - (second_half ? B.y - A.y : 0)) / segment_height;

        xI = alpha * C.x + A.x * (1 - alpha);
        yI = alpha * C.y + A.y * (1 - alpha);
        zI = alpha * C.depth + A.depth * (1 - alpha);

        xJ = second_half ? (beta * C.x + B.x * (1 - beta)) : beta * B.x + A.x * (1 - beta);
        yJ = second_half ? (beta * C.y + B.y * (1 - beta)) : beta * B.y + A.y * (1 - beta);
        zJ = second_half ? (beta * C.depth + B.depth * (1 - beta)) : beta * B.depth + A.depth * (1 - beta);

        if (xI > xJ)
        {
            swap_doubles(&xI, &xJ);
            swap_doubles(&yI, &yJ);
        }

        for (j = xI; j <= xJ; ++j)
        {
            if (j <= (x_viewpoint_min) || j >= (x_viewpoint_max) || (A.y + i) <= (y_viewpoint_min) || (A.y + i) >= (y_viewpoint_max))
                continue;
            phi = xJ == xI ? 1. : (float)(j - xI) / (float)(xJ - xI);
            Pz = zI + (zJ - zI) * phi;
            idx = j + Yvp_range * (A.y + i);
            if ((zbuffer + idx) == NULL || idx < 0 || idx > (Xvp_range + 1) * (Yvp_range + 1))
            {
                continue;
            }
            else if (*(zbuffer + idx) > Pz && *(zbuffer + idx) <= 20000 && *(zbuffer + idx) > -1000)
            {
                draw_pixel(j, A.y + i, Cl);
                *(zbuffer + idx) = Pz;
            }
        }
    }
}

int proect_coordinate(char flag, struct vertice *vertex, int num)
{
    switch (flag)
    {
    case 'x':
        return (int)(vertex[num].x * d / vertex[num].z + c1);
    case 'y':
        return (int)(vertex[num].y * d / vertex[num].z + c2);
    default:
        return 1;
    }
}

void normalize_vector(struct v3D *vec)
{
    double r;
    r = sqrt(sqr(vec->x) + sqr(vec->y) + sqr(vec->z));
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

void count_coefficients(double rho, double theta, double phi)
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

void update_z_buffer()
{
    int i;
    for (i = 0; i < (Xvp_range + 1) * (Yvp_range + 1); ++i)
    {
        if ((zbuffer + i) != NULL)
            *(zbuffer + i) = 20000;
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