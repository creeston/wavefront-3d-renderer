#include "canvas_drawing.h"
#include "model.h"

enum shading_type
{
    NO_SHADING,
    FLAT,
    GOURAUD,
};

void initialize_renderer();
void set_object_to_render(struct obj *object);
void set_color(struct color color, struct color edge_color);
void render();
void set_cartesian_translation(double alpha, double beta, double gamma);
void set_translation(int translation);
void set_scale(float scale);
void move_obj_relative(int dx, int dy);
void get_pixel_info(int x, int y);
int get_translation();
void set_draw_triangles(int should_draw_triangles);
void set_draw_vertices(int should_draw_vertices);
void set_draw_edges(int should_draw_edges_value);
void set_shading_type(enum shading_type shading_type);