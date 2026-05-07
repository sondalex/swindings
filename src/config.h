#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include "structures.h"

static const char PATTERN[] = "bindsym";

typedef enum {
    CONFIG_SUCCESS = 0,
    CONFIG_ERR_FILE_NOT_FOUND,
    CONFIG_ERR_PERMISSION_DENIED,
    CONFIG_ERR_OUT_OF_MEMORY,
    CONFIG_ERR_READ_ERROR,
    CONFIG_ERR_INVALID_FORMAT,
    CONFIG_ERR_INVALID_ARGUMENT,
    CONFIG_ERR_ALLOC_FAILED,
} config_error_t;

typedef struct {
    char *main_key;   // "mod", "Return", "q", etc.
    char **modifiers; // secondary keys: "Shift", "Ctrl", etc.
    size_t modifier_count;
    char *description; // "Focus left", "Kill window", etc.
} KeyMap;

typedef struct {
    KeyMap *items;
    size_t count;
    size_t capacity;
} KeyMapList;

// Parses sway bindsym lines into key maps.
// Returns 0 on success, -1 on error.
// "bindsym $mod+Shift+h move left"
//          ^^^^^^^^^^^  ^^^^^^^^^
//          keys         description
config_error_t parse_key_maps(stringlist_t *list, KeyMapList *out);

void keymaplist_init(KeyMapList *list);

// Appends a KeyMap to the list. Returns 0 on success, -1 on error.
config_error_t keymaplist_append(KeyMapList *list, KeyMap km);

// Frees all memory owned by the list and resets it to empty.
void keymaplist_free(KeyMapList *list);

// Frees all memory owned by the KeyMap.
void keymap_free(KeyMap *km);


config_error_t config_read_file(const char *filepath, stringlist_t *out);

// Returns the path to the sway config file, or NULL on error.
// Caller must free() the returned string.
char *config_get_sway_filepath(void);

#endif
