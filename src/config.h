#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

static const char PATTERN[] = "bindsym";
typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} StringList;

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
int parse_key_maps(StringList *list, KeyMapList *out);

void keymaplist_init(KeyMapList *list);

// Appends a KeyMap to the list. Returns 0 on success, -1 on error.
int keymaplist_append(KeyMapList *list, KeyMap km);

// Frees all memory owned by the list and resets it to empty.
void keymaplist_free(KeyMapList *list);

// Frees all memory owned by the KeyMap.
void keymap_free(KeyMap *km);

int stringlist_append(StringList *list, const char *s);

void stringlist_init(StringList *list);

int read_file(const char *filepath, StringList *out);

void stringlist_free(StringList *list);

// Returns the path to the sway config file, or NULL on error.
// Caller must free() the returned string.
char *get_sway_config_filepath(void);

#endif
