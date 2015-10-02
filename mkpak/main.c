#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <endian.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <argp.h>

#include "util.h"

#include "pak.h"

const char* argp_program_version = "Pak Creator 0.1";
const char* argp_program_bug_address = "<antidote.crk@gmail.com>";

static struct argp_option options[] = {
{"verbose", 'v', 0, 0, "Produce verbose output", 0},
{"make",    'm', 0, 0, "Create a pak from the specified directory, to the specified file", 0},
{"dump",    'd', 0, 0, "Dump a pak from the specified file to the specified directory", 0},
{"compress", 'c', 0, 0, "Compress each file before storing, if possible", 0},
{"info",     'i', 0, 0, "Print pak statistics and contents", 0},
{0}
};

static char doc[] =
        "\0";

/* A description of the arguments we accept. */
static char args_doc[] = "input [output]";

struct arguments {
    int verbose;
    bool make;
    bool abort;
    bool compress;
    bool info;
    char* input;
    char* output;
};


static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    struct arguments* arguments = state->input;
    switch(key) {
        case 'v':
            arguments->verbose = 1;
            break;
        case 'm':
            arguments->make = true;
            break;
        case 'd':
            arguments->make = false;
            break;
        case 'c':
            arguments->compress = true;
            break;
        case 'i':
            arguments->info = true;
            if (state->next + 1 > state->argc) {
                arguments->abort = true;
                return ARGP_KEY_ERROR;
            }
            arguments->input = state->argv[state->next];
            break;
        case ARGP_KEY_NO_ARGS:
            argp_usage(state);
        case ARGP_KEY_ARG:
            if (arg) {
                if (!arguments->input)
                    arguments->input = arg;
                else
                    arguments->output = arg;
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

struct argp argp_object = {options, parse_opt, args_doc, doc, NULL, NULL, NULL};

int main(int argc, char* argv[]) {
    struct arguments args;
    memset(&args, 0, sizeof(struct arguments));
    argp_parse(&argp_object, argc, argv, 0, 0, &args);

    if (args.abort || !args.input)
        return EXIT_FAILURE;
    if (args.info) {
        print_pak_info(args.input);
    } else if (!args.make) {
        if (!args.output) {
            printf("Dump Mode: Missing output directory\n");
            printf("Rerun with -? for more information\n");
            return EXIT_FAILURE;
        }
        dump_pak(args.input, args.output, args.verbose);
    } else {
        if (!args.output) {
            printf("Create Mode: Missing output file\n");
            printf("Rerun with -? for more information\n");
            return EXIT_FAILURE;
        }
        make_pak(args.input, args.output, args.compress, args.verbose);
    }
    return EXIT_SUCCESS;
}

