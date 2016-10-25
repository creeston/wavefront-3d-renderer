#include "renderer.h"
struct color Red = {255, 0 ,0};
struct v3D lightdir = {0,0,1};
double v11, v12, v13, v21, v22, v23, v32, v33, v43, d, c1, c2, onetime = 1, Xvp_range, Yvp_range;
double eps = 1.e-5, meps = -1.e-5, oneminus = 1 - 1.e-5, oneplus = 1 + 1.e-5;
int *zbuffer;

void read_object(char *filename, struct obj *object) {
    FILE *fpin;
    char string[100];

    struct ver *fvertex;
    struct ver *vertex;
    int nvr = 1;

    struct tr *triangle;
    int ntr = 0;

    fpin = fopen(filename, "r");
    while(!feof(fpin)) {
        fgets(string, sizeof(string), fpin);
        switch (string[0]) {
            case 'v':
                if (string[1] == ' ') {
                    string[strlen(string) - 1] = '\0';
                    read_vertex(string, &fvertex, &nvr);
                }
                break;
            case 'f':
                string[strlen(string) - 1] = '\0';
                read_face(string, &triangle, &ntr);
                break;
            default: continue; break;
        }
    }
    init_vertexes(&vertex, nvr);
    object->fvertex = fvertex; object->vertex = vertex; object->ntr = ntr; object->nvr = nvr; object->triangle = triangle;
    fclose(fpin);
}

void draw_obj(struct obj *object, struct color C, double theta, double phi) {
    update_z_buffer();
    struct ver **fvertex = &(object->fvertex);
    struct ver **vertex = &(object->vertex);
    int nvr = object->nvr;

    struct tr **triangle = &object->triangle;
    int ntr = object->ntr;

    double Range[4] = {BIG, -BIG, BIG, -BIG};
    count_coefficients(3000.0, theta, phi);

    calculate_vertexes(*fvertex, vertex, nvr, Range);

    if (onetime) {
        calculate_scale(Range[0], Range[1], Range[2], Range[3]);
        init_z_buffer();
    }
    onetime = 0;

    calculate_triangles(*vertex, triangle, ntr, nvr);
        /* Считаем нормали к вершинам*/
       //extraCalcVertex(vertex, nvr, *triangle);

    draw_triangle(*triangle, ntr, *vertex, C);
}

void draw_triangle(struct tr* triangle, int ntr, struct ver* vertex, struct color Cl) {
    int i, XA, YA, XB, YB, XC, YC, A, B, C;
    double a, b, c, h, intensity;
    normalize_vector(&lightdir);
    for (i = 0; i < ntr; ++i) {
        a = triangle[i].a; b = triangle[i].b; c = triangle[i].c; h = triangle[i].h;
        A = triangle[i].A; B = triangle[i].B; C = triangle[i].C;

        intensity = lightdir.x * a + lightdir.y * b + lightdir.z * c;
        if (h > 0 && intensity > 0) {
            struct color rgb = {intensity * Cl.r, intensity * Cl.g, intensity * Cl.b};

            if (A > ntr || B > ntr || C > ntr)
                continue;
            XA = proect_coordinate('x', vertex, A); YA = proect_coordinate('y', vertex, A);
            XB = proect_coordinate('x', vertex, B); YB = proect_coordinate('y', vertex, B);
            XC = proect_coordinate('x', vertex, C); YC = proect_coordinate('y', vertex, C);

            struct v2D vA = {XA, YA, vertex[A].z};
            struct v2D vB = {XB, YB, vertex[B].z};
            struct v2D vC = {XC, YC, vertex[C].z};

            rast_triangles(vA,vB,vC,rgb);
        }
    }
}

void init_z_buffer() {
    int i;
    zbuffer = (int *)malloc((Xvp_range + 1) * (Yvp_range + 1) * sizeof(int));
    for (i = 0; i < (Xvp_range + 1) * (Yvp_range + 1); ++i)
            *(zbuffer + i) = 20000;
}

void calculate_vertexes(struct ver* fvertex, struct ver** vertex, int nvr, double *Range) {
    int i;
    double x, y, z, xe, ye, ze, X, Y;
    double xO = fvertex[0].x, yO = fvertex[0].y, zO = fvertex[0].z;
    for (i = 1; i <= nvr; ++i) {
        x = (fvertex + i)->x; y = (fvertex + i)->y; z = (fvertex + i)->z;

        //преобразуем координаты (через матрицу поворота)
        viewing(x,xO, y,yO, z,zO, &xe, &ye, &ze);
        if (ze <= eps)
            continue;

        //находим max и min для масштабирования в наше окно вывода
        X = xe / ze; Y = ye / ze;

        if (X < Range[0]) Range[0] = X; if (X > Range[1]) Range[1] = X;   //Range = {Xmin, Xmax, Ymin, Ymax}
        if (Y < Range[2]) Range[2] = Y; if (Y > Range[3]) Range[3] = Y;

        (*vertex + i)->x = xe; (*vertex + i)->y = ye; (*vertex + i)->z = ze;
  if ((*vertex + i) == NULL)
      printf("some kind of error\n");
    }
}

void calculate_scale(double Xmin, double Xmax, double Ymin, double Ymax) {
    double fx, fy, Xcentre, Ycentre, Xvp_centre, Yvp_centre;
    double Xrange, Yrange;

    Xrange = Xmax - Xmin; Yrange = Ymax - Ymin;
    Xvp_range = Xvp_max - Xvp_min; Yvp_range = Yvp_max - Yvp_min;
    fx = Xvp_range / Xrange; fy = Yvp_range / Yrange;
    d = (fx < fy) ? fx : fy;
    Xcentre = (Xmin + Xmax) / 2; Ycentre = (Ymax + Ymin) / 2;
    Xvp_centre = (Xvp_min + Xvp_max) / 2; Yvp_centre = (Yvp_min + Yvp_max) / 2;
    c1 = Xvp_centre - d * Xcentre; c2 = Yvp_centre - d * Ycentre;
}

void calculate_triangles(struct ver* vertex, struct tr **triangle, int ntr, int nvr) {
    double xA, yA, zA, xB, yB, zB, xC, yC, zC, a, b, c, h, r;
    int i, j, A, B, C;
    for (i = 0; i < ntr; ++i) {
        A = (*triangle + i)->A; B = (*triangle + i)->B; C = (*triangle + i)->C;
        /*После выполнения функции RotateObj / MoveObj обьект, видимо, немного повреждается
        и некоторые значения номеров треугольников очень сильно меняются
        поэтому необходима проверка их соотвествия действительности, иначе - SegFault*/
        if (A > nvr || A < 0 || B > nvr || B < 0 || C > nvr || C < 0) {
              continue;
        }
        if ((vertex + A) == NULL || (vertex + B) == NULL || (vertex + C) == NULL) {
          continue;
        }

        if ((*triangle + i)->first == 1) {
            xA = (vertex + A)->x; yA = (vertex + A)->y; zA = (vertex + A)->z;
            xB = (vertex + B)->x; yB = (vertex + B)->y; zB = (vertex + B)->z;
            xC = (vertex + C)->x; yC = (vertex + C)->y; zC = (vertex + C)->z;

            a = yA * (zB - zC) - yB * (zA - zC) + yC * (zA - zB);
            b = -(xA * (zB - zC) - xB * (zA - zC) + xC * (zA - zB));
            c =  xA * (yB - yC) - xB * (yA - yC) + xC * (yA - yB);
            h = xA * (yB * zC - yC * zB) - xB * (yA * zC - yC * zA) + xC * (yA * zB - yB * zA);

            r = sqrt(a*a + b*b + c*c);
            a /= r; b /= r; c /= r; h /= r;

            (*triangle + i)->a = a; (*triangle + i)->b = b; (*triangle + i)->c = c;
            (*triangle + i)->h = h;

        } else {
            j = i;
            while ((*triangle + j)->first != 1 )
              --j;
            (*triangle + i)->a = (*triangle + j)->a; (*triangle + i)->b = (*triangle + j)->b; (*triangle + i)->c = (*triangle + j)->c;
            (*triangle + i)->h = (*triangle + j)->h;
        }
    }
}

void rast_triangles(struct v2D A, struct v2D B, struct v2D C, struct color Cl) {
    if (A.y == B.y && A.y == C.y) 
      return;
    if (A.y > B.y) swapS(&A,&B);
    if (A.y > C.y) swapS(&A,&C);
    if (B.y > C.y) swapS(&B,&C);

    int total_height = C.y - A.y, i, second_half, j, idx, segment_height;
    float alpha, beta, phi;
    double xI, yI, zI, xJ, yJ, zJ, Pz;
    long double time_spent_on_calculating = 0, time_spent_on_drawing = 0;
    for (i = 0; i <= total_height; ++i) {
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

        if (xI > xJ) {
            swap(&xI, &xJ); swap(&yI, &yJ);
        }

        for (j = xI; j <= xJ; ++j) {
            if (j <= (Xvp_min) || j >= (Xvp_max) || (A.y + i) <= (Yvp_min) || (A.y + i) >= (Yvp_max) ) //будем считать, что окно вывода [50 350 50 350]
                continue;
            phi = xJ == xI ? 1. : (float)(j-xI) / (float)(xJ-xI);
            Pz = zI + (zJ - zI) * phi;
            //idx = Px + 320 * Py;
            idx = j + Yvp_range * (A.y + i);
            if ((zbuffer + idx) == NULL || idx < 0 || idx > (Xvp_range + 1) * (Yvp_range + 1)) { 
                continue; 
            } //idx иногда принимает весьма странные значения
            else if (*(zbuffer + idx) > Pz && *(zbuffer + idx) <= 20000 && *(zbuffer + idx) > -1000) {
                //!*(zbuffer + idx) <= 20000 очень важный по-видимому момент(нет, строчка выше - вот важный момент), без него SegFault выдавало
                draw_pixel(j, A.y + i, Cl);
                *(zbuffer + idx) = Pz;
            }
        }
    }
}

void read_vertex(char *str, struct ver **fvertex, int *nvr) {
    static double xmin = BIG, ymin = BIG, zmin = BIG, xmax = -BIG, ymax = -BIG, zmax = -BIG;
    char numx[15], numy[15], numz[15];
    int i = 1;
    while (str[i] == ' ')
        ++i;
    int j = 0;
    while (str[i] != ' ') {
        numx[j] = str[i];
        ++i; ++j;
    }
    double x = atof(numx) * 100;
    ++i; 
    j = 0;
    while (str[i] != ' ') {
        numy[j] = str[i];
        ++i; ++j;
    }
    double y = atof(numy) * 100;
    ++i; j = 0;
    while (str[i] != '\0') {
        numz[j] = str[i];
       ++i; ++j;
    }
    double z = atof(numz) * 100;

    if (x > xmax) xmax = x; if (x < xmin) xmin = x;
    if (y > ymax) ymax = y; if (y < ymin) ymin = y;
    if (z > zmax) zmax = z; if (z < zmin) zmin = z;

    if (*nvr == 1) {
        *fvertex = (struct ver *)malloc(2 * sizeof(struct ver));
        (*fvertex)[0].x = 0; (*fvertex)[0].y = 0; (*fvertex)[0].z = 0;
    } else
        *fvertex = (struct ver *)realloc(*fvertex, (*nvr + 1) * sizeof(struct ver));

    (*fvertex)->x = (xmin + xmax) / 2; (*fvertex)->y = (ymin + ymax) / 2; (*fvertex)->z = (zmin + zmax) / 2;
    (*fvertex + *nvr)->x = x; (*fvertex + *nvr)->y = y; (*fvertex + *nvr)->z = z;
    *nvr += 1;
}

void read_face(char *str, struct tr **triangle, int *ntr) {
    char num[10], part[150][30];
    int j = 0, i = 2, k = 0, poly[30];

    while (1 == 1) {
        if (str[i] == ' ') {
            part[k][j] = '\0';
            if (str[i + 1] == '\0') break;
            ++i; ++k; j = 0;
            continue;
        } else if (str[i] == '\0') {
            part[k][j] = '\0';
            break;
        }
        part[k][j] = str[i];
        ++j; ++i;
    }

    for (i = 0; i <= k; ++i) {
        j = 0;
        while (part[i][j] != '/' && part[i][j] != '\0') {
            num[j] = part[i][j];
            ++j;
        }
        poly[i] = abs(atoi(num));
        while (j >= 0) {
            num[j] = '\0';
            --j;
        }
    }

    int i0 = 0, i1 = 1, i2 = 2, A, B, C, first = 1;
    while(i2 != k + 1) {
        A = poly[i0], B = poly[i1], C = poly[i2];
        if (*ntr == 0)
            { *triangle = malloc(sizeof(struct tr));}
        else
           *triangle = (struct tr*)realloc(*triangle, (*ntr + 1) * sizeof(struct tr));

        (*triangle + *ntr)->A = A; (*triangle + *ntr)->B = B; (*triangle + *ntr)->C = C; (*triangle + *ntr)->first = first;
        *ntr += 1; ++i1; ++i2; first = 0;
    }
}

void init_vertexes(struct ver **vertex, int nvr) {   
    int i;
    *vertex = (struct ver*)malloc((nvr + 1) * sizeof(struct ver));
    for (i = 1; i <= nvr; ++i) {
        (*vertex + i)->connect = (int *)malloc(100 * sizeof(int));
        *((*vertex + i)->connect) = 0;
    }
}

int proect_coordinate(char flag, struct ver* vertex, int num) {
    switch (flag) {
    case 'x': 
        return (int)( vertex[num].x * d / vertex[num].z + c1);
    case 'y': 
        return (int)( vertex[num].y * d / vertex[num].z + c2);
    default: 
        return 1;
    }
}

void normalize_vector(struct v3D *vec) {
    double r;
    r = sqrt(sqr(vec->x) + sqr(vec->y) + sqr(vec->z));
    vec->x /= r; vec->y /= r; vec->z /= r;
}

void viewing(double x, double xO,double y, double yO, double z, double zO, double *pxe, double *pye, double *pze) {
    *pxe = (v11 * (x - xO) + v21 * (y - yO)) + xO;
    *pye = (v12 * (x - xO) + v22 * (y - yO) + v32 * (z - zO)) + yO;
    *pze = (v13 * (x - xO)+ v23 * (y - yO) + v33 * (z - zO) + v43) + zO;
}

void count_coefficients(double rho, double theta, double phi) {
    double th, ph, costh, sinth, cosph, sinph, factor;

    factor = atan(1.0) / 45.0;
    th = theta * factor; ph = phi * factor;
    costh = cos(th); sinth = sin(th);
    cosph = cos(ph); sinph = sin(ph);

    v11 = -sinth; v12 = -cosph * costh; v13 = -sinph * costh;
    v21 = costh;  v22 = -cosph * sinth; v23 = -sinph * sinth;
                  v32 = sinph;          v33 = -cosph;
                                        v43 = rho;
}

void update_z_buffer() {
    int i;
    for (i = 0; i < (Xvp_range + 1) * (Yvp_range + 1); ++i) {
        if ((zbuffer + i) != NULL)
            *(zbuffer + i) = 20000;
    }
}

void rast_line(struct v2D A, struct v2D B, struct v2D C, struct color Cl) {
    line(A.x, A.y, B.x, B.y, Cl, CROP); line(B.x, B.y, C.x, C.y, Cl, CROP); line(C.x, C.y, A.x, A.y, Cl, CROP);
}