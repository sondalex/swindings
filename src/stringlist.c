#include "stringlist.h"

#include <stdlib.h>
#include <string.h>

stringlist_error_t stringlist_append(StringList *list, const char *s) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 8;
        char **tmp = (char **)realloc(list->items, new_cap * sizeof(*tmp));
        if (!tmp)
            return STRINGLIST_ERR_ALLOC_FAILED;
        list->items = tmp;
        list->capacity = new_cap;
    }
    list->items[list->count] = strdup(s);
    if (!list->items[list->count])
        return -1;
    list->count++;
    return 0;
}

void stringlist_init(StringList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void stringlist_free(StringList *list) {
    for (size_t i = 0; i < list->count; i++)
        free(list->items[i]);
    free(list->items);
    stringlist_init(list);
}
