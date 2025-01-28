#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void read_vertex(char *str, struct obj_vertex **vertices, int *vertex_count);
void read_face(char *str, struct obj_triangle **triangles, int *triangles_count);
int validate_object(struct obj *object);

#define LINE_BUFFER_SIZE 256
#define MAX_NUM_LENGTH 32
#define SQR(x) ((x) * (x))
#define MAX_POLY_INDICES 30

void read_object(const char *filename, struct obj *object)
{
    if (!filename || !object)
    {
        fprintf(stderr, "Invalid arguments to read_object\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    struct obj_vertex *vertices = NULL;
    struct obj_triangle *triangles = NULL;
    int vertex_count = 1, triangle_count = 0;

    char line[LINE_BUFFER_SIZE];

    while (fgets(line, sizeof(line), file))
    {
        switch (line[0])
        {
        case 'v':
            if (line[1] == ' ')
            {
                read_vertex(line, &vertices, &vertex_count);
            }
            break;
        case 'f':
            read_face(line, &triangles, &triangle_count);
            break;
        default:
            continue;
            break;
        }
    }
    fclose(file);

    object->vertices = vertices;
    object->triangles = triangles;
    object->number_of_triangles = triangle_count;
    object->number_of_vertices = vertex_count;

    if (!validate_object(object))
    {
        fprintf(stderr, "Invalid object data\n");
        if (object->vertices != NULL)
        {
            free(object->vertices);
        }

        if (object->triangles != NULL)
        {
            free(object->triangles);
        }

        exit(EXIT_FAILURE);
    }

    double xmin = BIG, ymin = BIG, zmin = BIG, xmax = -BIG, ymax = -BIG, zmax = -BIG;

    for (int i = 1; i < object->number_of_vertices; i++)
    {
        double x = object->vertices[i].x;
        double y = object->vertices[i].y;
        double z = object->vertices[i].z;

        if (x > xmax)
        {
            xmax = x;
        }
        if (x < xmin)
        {
            xmin = x;
        }
        if (y > ymax)
        {
            ymax = y;
        }
        if (y < ymin)
        {
            ymin = y;
        }
        if (z > zmax)
        {
            zmax = z;
        }
        if (z < zmin)
        {
            zmin = z;
        }
    }

    double x_origin = (xmin + xmax) / 2;
    double y_origin = (ymin + ymax) / 2;
    double z_origin = (zmin + zmax) / 2;

    for (int i = 1; i < object->number_of_vertices; i++)
    {
        object->vertices[i].x -= x_origin;
        object->vertices[i].y -= y_origin;
        object->vertices[i].z -= z_origin;
    }

    object->vertices[0].x = 0;
    object->vertices[0].y = 0;
    object->vertices[0].z = 0;

    object->largest_dimension = sqrt(SQR(xmax - xmin) + SQR(ymax - ymin) + SQR(zmax - zmin));
    object->x_min = xmin;
    object->x_max = xmax;
    object->y_min = ymin;
    object->y_max = ymax;
    object->z_min = zmin;
    object->z_max = zmax;
}

void read_vertex(char *line, struct obj_vertex **vertices, int *vertex_count)
{
    if (!line || !vertices || !vertex_count)
    {
        fprintf(stderr, "Invalid arguments to read_vertex\n");
        exit(EXIT_FAILURE);
    }

    double x, y, z;
    if (sscanf(line, "v %lf %lf %lf", &x, &y, &z) != 3)
    {
        fprintf(stderr, "Invalid vertex line: %s\n", line);
        exit(EXIT_FAILURE);
    }

    int SCALING_FACTOR = 100;
    x *= SCALING_FACTOR;
    y *= SCALING_FACTOR;
    z *= SCALING_FACTOR;

    if (*vertex_count == 1)
    {
        *vertices = (struct obj_vertex *)malloc(2 * sizeof(struct obj_vertex));
        (*vertices)[0].x = 0;
        (*vertices)[0].y = 0;
        (*vertices)[0].z = 0;
    }
    else
    {
        *vertices = (struct obj_vertex *)realloc(*vertices, (*vertex_count + 1) * sizeof(struct obj_vertex));
    }

    if (!*vertices)
    {
        perror("Memory allocation failed for vertices");
        exit(EXIT_FAILURE);
    }

    (*vertices)[*vertex_count].x = -x;
    (*vertices)[*vertex_count].y = -y;
    (*vertices)[*vertex_count].z = -z; // TODO ?
    (*vertex_count)++;
}

void read_face(char *line, struct obj_triangle **triangles, int *triangle_count)
{
    if (!line || !triangles || !triangle_count)
    {
        fprintf(stderr, "Invalid arguments to read_face\n");
        exit(EXIT_FAILURE);
    }

    int poly_indices[MAX_POLY_INDICES];
    int vertex_count = 0;
    char *ptr = line + 2;
    const char *separator = " ";
    char *vertex_triple = strtok(ptr, separator); // obj vertex description: vertex/texture/normal
    while (vertex_triple != NULL)
    {
        int vertex_index;
        int res = sscanf(vertex_triple, "%d", &vertex_index);
        if (res == -1)
        {
            break;
        }
        poly_indices[vertex_count] = vertex_index;
        vertex_triple = strtok(NULL, separator);
        vertex_count++;
    }

    int triangle_count_in_polygon = vertex_count - 2;
    int part_of_polygon = -1;
    for (int i = 0; i < triangle_count_in_polygon; i++)
    {
        if (*triangle_count == 0)
        {
            *triangles = malloc(sizeof(struct obj_triangle));
        }
        else
        {
            *triangles = (struct obj_triangle *)realloc(*triangles, (*triangle_count + 1) * sizeof(struct obj_triangle));
        }

        if (!*triangles)
        {
            perror("Memory allocation failed for triangles");
            exit(EXIT_FAILURE);
        }

        struct obj_triangle *triangle = (*triangles + *triangle_count);

        // Only the vertex index is needed
        triangle->vertex_a = poly_indices[0];
        triangle->vertex_b = poly_indices[i + 1];
        triangle->vertex_c = poly_indices[i + 2];
        triangle->part_of_polygon = part_of_polygon;

        if (part_of_polygon == -1)
        {
            part_of_polygon = *triangle_count; // Set the polygon reference
        }

        (*triangle_count)++;
    }
}

int validate_object(struct obj *object)
{
    if (object->number_of_vertices == 0 || object->number_of_triangles == 0)
    {
        return 0;
    }

    for (int i = 0; i < object->number_of_triangles; ++i)
    {
        struct obj_triangle triangle = object->triangles[i];
        if (triangle.vertex_a > object->number_of_vertices || triangle.vertex_b > object->number_of_vertices || triangle.vertex_c > object->number_of_vertices)
        {
            printf("Invalid triangle vertices\n");
            return 0;
        }
    }

    for (int i = 0; i < object->number_of_vertices; i++)
    {
        if ((object->vertices + i) == NULL)
        {
            printf("Invalid vertex\n");
            return 0;
        }
    }

    return 1;
}