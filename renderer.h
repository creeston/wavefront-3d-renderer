#include "canvas_drawing.h"
#include "model.h"

void initialize_renderer();
void render_obj(struct obj *object, struct color C, double theta, double phi, double rho);
void move_obj_relative(struct obj *object, int dx, int dy, int dz);
void move_obj_absolute(struct obj *object, int x, int y, int z);
void change_scale(float scale);