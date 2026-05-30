#include "cli.h"
#include "cargs.h"

#include <stdio.h>
#include <stdlib.h>

static struct cag_option options[] = {
    {.identifier = 'h',
     .access_letters = "h",
     .access_name = "help",
     .value_name = NULL,
     .description = "Shows this help message"},

    {.identifier = 'v',
     .access_letters = "v",
     .access_name = "version",
     .value_name = NULL,
     .description = "Shows the version"},

    {.identifier = 'c',
     .access_letters = "c",
     .access_name = "config",
     .value_name = "FILE",
     .description = "Path to a theme configuration file (TOML)"},
    {.identifier = 's',
     .access_letters = "s",
     .access_name = "sway-config",
     .value_name = "FILE",
     .description = "Path to sway config. Defaults to None. When None, sway "
                    "path resolution is followed."}};

static void init_args(cli_args *args) {
    args->help = false;
    args->version = false;
    args->config = NULL;
    args->sway_config = NULL;
}

bool parse_cli(int argc, char **argv, cli_args *args) {
    init_args(args);

    cag_option_context context;
    cag_option_init(&context, options, CAG_ARRAY_SIZE(options), argc, argv);

    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'h':
            printf("Usage: swindings [OPTION]...\n");
            printf("A keybinding viewer/cheatsheet for Sway.\n\n");
            cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
            args->help = true;
            return false;
        case 'v':
            printf("swindings %s\n", GIT_VERSION);
            args->version = true;
            return false;
        case 'c':
            args->config = (char *)cag_option_get_value(&context);
            break;
        case 's':
            args->sway_config = (char *)cag_option_get_value(&context);
            break;
        case '?':
            cag_option_print_error(&context, stderr);
            return false;
        default:
            break;
        }
    }

    return true;
}
