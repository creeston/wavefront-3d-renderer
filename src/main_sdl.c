#include <argp.h>
#include <stdlib.h>
#include "../include/gui.h"

const char *argp_program_version = "wavefront-obj-renderer 1.0";
const char *argp_program_bug_address = "<mityy2012@gmail.com>";

static char doc[] = "Wavefront 3D OBJ Renderer -- A program to render 3D objects in Wavefront OBJ format using GTK.";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"file", 'f', "FILE", 0, "Path to the Wavefront .obj file [REQUIRED]"},
    {"width", 'w', "WIDTH", 0, "Width of the canvas [DEFAULT: 800]"},
    {"height", 'h', "HEIGHT", 0, "Height of the canvas [DEFAULT: 600]"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key)
    {
    case 'f':
        arguments->file = arg;
        break;
    case 'w':
        arguments->width = atoi(arg);
        break;
    case 'h':
        arguments->height = atoi(arg);
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num > 0)
        {
            argp_usage(state);
        }
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 0 || !arguments->file)
        {
            argp_failure(state, 1, 0, "required --file. See --help for more information");
            exit(ARGP_ERR_UNKNOWN);
        }
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char **argv)
{
    struct arguments arguments;

    arguments.file = NULL;
    arguments.height = 600;
    arguments.width = 800;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    init_gui(arguments, argc, argv);
    cleanup_gui();
    return 0;
}
