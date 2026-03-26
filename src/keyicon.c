#include "keyicon.h"
#include <string.h>

typedef struct {
    const char *name;
    const char *symbol;
    int codepoint;
} KeySymbol;

static const KeySymbol key_symbols[] = {
    {"Shift", "⇧", 0x21E7},     {"Ctrl", "⌃", 0x2303},   {"Alt", "⌥", 0x2325},
    {"Return", "⏎", 0x23CE},    {"Escape", "⎋", 0x238B}, {"Tab", "⇥", 0x21E5},
    {"Up", "↑", 0x2191},        {"$up", "↑", 0x2191},    {"Down", "↓", 0x2193},
    {"$down", "↓", 0x2193},     {"Left", "←", 0x2190},   {"$left", "←", 0x2190},
    {"Right", "→", 0x2192},     {"$right", "→", 0x2192}, {"space", "␣", 0x2423},
    {"BackSpace", "⌫", 0x232B}, {"Delete", "⌦", 0x2326}, {"$mod", "⌘", 0x2318},

};

const int key_codepoints[] = {0x21E7, 0x2303, 0x2325, 0x23CE, 0x238B,
                              0x21E5, 0x2191, 0x2193, 0x2190, 0x2192,
                              0x2423, 0x232B, 0x2326, 0x2318};
const int key_codepoints_count =
    sizeof(key_codepoints) / sizeof(key_codepoints[0]);

const char *key_to_symbol(const char *key) {
    size_t n = sizeof(key_symbols) / sizeof(key_symbols[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcmp(key, key_symbols[i].name) == 0)
            return key_symbols[i].symbol;
    }
    return key;
}
