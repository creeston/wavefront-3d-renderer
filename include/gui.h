struct arguments
{
    char *file;
    int width;
    int height;
};

void init_gui(struct arguments arguments, int argc, char **argv);
void resize_canvas(int width, int height);
void set_file(const char *url);
void cleanup_gui();