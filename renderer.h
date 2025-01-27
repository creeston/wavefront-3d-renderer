#include "canvas_drawing.h"
#include "model.h"

void initialize_renderer();
void set_object_to_render(struct obj *object);
void set_color(struct color color);
void render();
void set_rotation_angles(double theta, double phi, double rho);
void set_cartesian_translation(double alpha, double beta, double gamma);
void set_translation(int translation);
void set_scale(float scale);
void move_obj_relative(int dx, int dy);
void get_pixel_info(int x, int y);
int get_translation();