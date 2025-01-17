#include "canvas_drawing.h"
#include "model.h"

void initialize_renderer();
void set_object_to_render(struct obj *object);
void set_color(struct color color);
void render();
void set_rotation_angles(double theta, double phi, double rho);
void set_scale(float scale);
void move_obj_relative(struct obj *object, int dx, int dy, int dz);
void move_obj_absolute(struct obj *object, int x, int y, int z);