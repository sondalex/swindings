#include "./theme.h"
#include <stdbool.h>

static char *theme_get_config_path() {}

static bool file_exists(char *filepath) {
    // TODO: Implement 
    return true;
}

static void create_file(char *filepath) {}

static Status theme_parse_file(char *filepath) {}

Status theme_set_from_config(Theme *theme) {

    char *filepath = theme_get_config_filepath();

    if (!file_exists(filepath)) {
        create_file(filepath);
    }

    theme_parse_file(filepath);
}
