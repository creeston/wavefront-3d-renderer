#include "canvas_drawing.h"
#include "model.h"

void initialize_renderer();
void render(struct obj *object, struct color color);
void set_rotation_angles(double theta, double phi, double rho);
void set_scale(float scale);
void move_obj_relative(struct obj *object, int dx, int dy, int dz);
void move_obj_absolute(struct obj *object, int x, int y, int z);