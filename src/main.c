#include "config.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    StringList list;
    stringlist_init(&list);

    char *filepath = get_sway_config_filepath();
    if (!filepath) {
        fprintf(stderr, "failed to determine sway config path\n");
        return 1;
    }

    if (read_file(filepath, &list) != 0) {
        fprintf(stderr, "failed to read file\n");
        free(filepath);
        return 1;
    }

    free(filepath);
    KeyMapList kml;
    keymaplist_init(&kml);

    if (parse_key_maps(&list, &kml) != 0) {
        fprintf(stderr, "failed to parse key maps\n");
        stringlist_free(&list);
        return 1;
    }

    display(&kml);
    stringlist_free(&list);

    keymaplist_free(&kml);
    return 0;
}
