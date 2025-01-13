#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_vertex(char *str, struct vertice **fvertex, int *nvr);
void read_face(char *str, struct triangle **triangle, int *ntr);
void init_vertexes(struct vertice **vertex, int nvr);

void read_object(char *filename, struct obj *object)
{
    FILE *fpin;
    char string[100];

    struct vertice *fvertex;
    struct vertice *vertex;
    int nvr = 1;

    struct triangle *triangle;
    int ntr = 0;

    fpin = fopen(filename, "r");
    while (!feof(fpin))
    {
        fgets(string, sizeof(string), fpin);
        switch (string[0])
        {
        case 'v':
            if (string[1] == ' ')
            {
                string[strlen(string) - 1] = '\0';
                read_vertex(string, &fvertex, &nvr);
            }
            break;
        case 'f':
            string[strlen(string) - 1] = '\0';
            read_face(string, &triangle, &ntr);
            break;
        default:
            continue;
            break;
        }
    }
    init_vertexes(&vertex, nvr);
    object->fvertex = fvertex;
    object->vertex = vertex;
    object->ntr = ntr;
    object->nvr = nvr;
    object->triangle = triangle;
    fclose(fpin);
}

void read_vertex(char *str, struct vertice **fvertex, int *nvr)
{
    static double xmin = BIG, ymin = BIG, zmin = BIG, xmax = -BIG, ymax = -BIG, zmax = -BIG;
    char numx[15], numy[15], numz[15];
    int i = 1;
    while (str[i] == ' ')
        ++i;
    int j = 0;
    while (str[i] != ' ')
    {
        numx[j] = str[i];
        ++i;
        ++j;
    }
    double x = atof(numx) * 100;
    ++i;
    j = 0;
    while (str[i] != ' ')
    {
        numy[j] = str[i];
        ++i;
        ++j;
    }
    double y = atof(numy) * 100;
    ++i;
    j = 0;
    while (str[i] != '\0')
    {
        numz[j] = str[i];
        ++i;
        ++j;
    }
    double z = atof(numz) * 100;

    if (x > xmax)
        xmax = x;
    if (x < xmin)
        xmin = x;
    if (y > ymax)
        ymax = y;
    if (y < ymin)
        ymin = y;
    if (z > zmax)
        zmax = z;
    if (z < zmin)
        zmin = z;

    if (*nvr == 1)
    {
        *fvertex = (struct vertice *)malloc(2 * sizeof(struct vertice));
        (*fvertex)[0].x = 0;
        (*fvertex)[0].y = 0;
        (*fvertex)[0].z = 0;
    }
    else
        *fvertex = (struct vertice *)realloc(*fvertex, (*nvr + 1) * sizeof(struct vertice));

    (*fvertex)->x = (xmin + xmax) / 2;
    (*fvertex)->y = (ymin + ymax) / 2;
    (*fvertex)->z = (zmin + zmax) / 2;
    (*fvertex + *nvr)->x = x;
    (*fvertex + *nvr)->y = y;
    (*fvertex + *nvr)->z = z;
    *nvr += 1;
}

void read_face(char *str, struct triangle **triangle, int *ntr)
{
    char num[10], part[150][30];
    int j = 0, i = 2, k = 0, poly[30];

    while (1 == 1)
    {
        if (str[i] == ' ')
        {
            part[k][j] = '\0';
            if (str[i + 1] == '\0')
                break;
            ++i;
            ++k;
            j = 0;
            continue;
        }
        else if (str[i] == '\0')
        {
            part[k][j] = '\0';
            break;
        }
        part[k][j] = str[i];
        ++j;
        ++i;
    }

    for (i = 0; i <= k; ++i)
    {
        j = 0;
        while (part[i][j] != '/' && part[i][j] != '\0')
        {
            num[j] = part[i][j];
            ++j;
        }
        poly[i] = abs(atoi(num));
        while (j >= 0)
        {
            num[j] = '\0';
            --j;
        }
    }

    int i0 = 0, i1 = 1, i2 = 2, A, B, C, first = 1;
    while (i2 != k + 1)
    {
        A = poly[i0], B = poly[i1], C = poly[i2];
        if (*ntr == 0)
            *triangle = malloc(sizeof(struct triangle));
        else
            *triangle = (struct triangle *)realloc(*triangle, (*ntr + 1) * sizeof(struct triangle));

        (*triangle + *ntr)->A = A;
        (*triangle + *ntr)->B = B;
        (*triangle + *ntr)->C = C;
        (*triangle + *ntr)->first = first;
        *ntr += 1;
        ++i1;
        ++i2;
        first = 0;
    }
}

void init_vertexes(struct vertice **vertex, int nvr)
{
    int i;
    *vertex = (struct vertice *)malloc((nvr + 1) * sizeof(struct vertice));
    for (i = 1; i <= nvr; ++i)
    {
        (*vertex + i)->connect = (int *)malloc(100 * sizeof(int));
        *((*vertex + i)->connect) = 0;
    }
}