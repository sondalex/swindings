#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include "theme.h"

typedef struct {
    float y;
    float min;
} Scroll;

typedef enum {
    DISPLAY_SUCCESS = 0,
    DISPLAY_FONT_CONVERSION_ERROR,
    DISPLAY_FONT_LOAD_ERROR,
    DISPLAY_STRINGLIST_UNINITIALIZED,

} DisplayError;

typedef struct {
    const char *info;
    const char *search;
} labels_t;

void display(const KeyMapList *kml, const theme_t *theme);

#endif
