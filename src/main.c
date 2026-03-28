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

} Error;

int main(void) {
    StringList list;
    stringlist_init(&list);
    Theme theme;
    Error err = theme_set_from_config(&theme);
    if (err != 0) {
        fprintf(stderr, "Failed to set config, exit with code %u", err);
        return err ;
    };

    char *filepath = config_get_sway_filepath();
    if (!filepath) {
        fprintf(stderr, "failed to determine sway config path\n");
        return ConfigError;
    }

    if (config_read_file(filepath, &list) != 0) {
        fprintf(stderr, "failed to read file\n");
        free(filepath);
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
    return 0;
}
