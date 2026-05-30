#ifndef CLI_H
#define CLI_H

#ifndef GIT_VERSION
#define GIT_VERSION "unknown-version"
#endif

#include <stdbool.h>

typedef struct {
    bool help;
    bool version;
    char *config;
    char *sway_config;
    bool follow_includes;
} cli_args;

/**
 * Parse command-line arguments into the provided cli_args struct.
 *
 * Returns true if the program should continue execution,
 * or false if it should exit (e.g. after printing help/version).
 */
bool parse_cli(int argc, char **argv, cli_args *args);

#endif
