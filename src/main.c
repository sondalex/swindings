#include "cli.h"
#include "config.h"
#include "display.h"
#include "theme.h"

#include <stdio.h>
#include <stdlib.h>
// TODO: Include error/status struct for theme.c
// For display, for sway parsing

typedef enum {
    Success = 0,
    ConfigError,
    ThemeError,
} Error;

int main(int argc, char *argv[]) {
    cli_args args;
    if (!parse_cli(argc, argv, &args)) {
        return (args.help || args.version) ? Success : 1;
    }

    stringlist_t list;
    stringlist_init(&list);
    theme_t theme;
    theme_error_t err = THEME_SUCCESS;
    if (args.config) {
        theme_result_t res = theme_load(args.config);
        err = res.error;
        if (err == THEME_SUCCESS) {
            theme = res.theme;
        }
    } else {
        err = theme_load_from_config(&theme);
    }
    if (err != THEME_SUCCESS) {
        fprintf(stderr, "Failed to set THEME: %s\n", theme_error_str(err));
        return ThemeError;
    }

    char *filepath = config_get_sway_filepath();
    if (!filepath) {
        fprintf(stderr, "failed to determine sway config path\n");
        return ConfigError;
    }

    if (config_read_file(filepath, &list) != 0) {
        fprintf(stderr, "failed to read file\n");
        free(filepath);
        stringlist_free(&list);
        return ConfigError;
    }

    free(filepath);
    KeyMapList kml;
    keymaplist_init(&kml);

    if (parse_key_maps(&list, &kml) != 0) {
        fprintf(stderr, "failed to parse key maps\n");
        stringlist_free(&list);
        return 1;
    }

    display(&kml, &theme);
    stringlist_free(&list);

    keymaplist_free(&kml);
    theme_free(&theme);
    return 0;
}
