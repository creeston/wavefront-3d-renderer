#include "models.h"

void initialize_renderer();
void render();

void set_object_to_render(struct wavefront_obj object);
void set_colors(int background_color, struct color color, struct color edge_color);
void set_cartesian_translation(double alpha, double beta, double gamma);
void set_camera_distance(int value);
void set_scale(float scale_multiplier);
void set_shading_type(enum shading_type shading_type);
void move_obj_relative(int dx, int dy);

int get_camera_distance();