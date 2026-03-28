#ifndef STRINGLIST_H
#define STRINGLIST_H

#include <stddef.h>

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} StringList;

#endif
