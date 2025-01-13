#include <argp.h>
#include <stdlib.h>
#include "gtk_gui.h"

const char *argp_program_version = "3d-render 1.0";
const char *argp_program_bug_address = "<bug@example.com>";

static char doc[] = "3D Renderer -- A program to render 3D objects using GTK.";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"file", 'f', "FILE", 0, "Path to the 3D object file"},
    {"fps", 'r', "FPS", 0, "Frames per second (30 or 60, default 60)"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key)
    {
    case 'f':
        arguments->file = arg;
        break;
    case 'r':
        arguments->fps = atoi(arg);
        if (arguments->fps != 30 && arguments->fps != 60)
        {
            argp_error(state, "FPS must be 30 or 60");
        }
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num > 0)
        {
            argp_usage(state);
        }
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 0)
        {
            argp_usage(state);
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
    arguments.fps = 60;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    init_gui(arguments, argc, argv);
    return 0;
}
