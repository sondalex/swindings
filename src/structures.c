#include "structures.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stringlist_error_t stringlist_append(stringlist_t *list, const char *s) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 8;
        char **tmp =
            (char **)realloc((char *)(list->items), new_cap * sizeof(*tmp));
        if (!tmp)
            return STRINGLIST_ERR_ALLOC_FAILED;
        list->items = tmp;
        list->capacity = new_cap;
    }

    list->items[list->count] = strdup(s);
    if (!list->items[list->count])
        return STRINGLIST_ERR_ALLOC_FAILED;

    list->count++;
    return STRINGLIST_SUCCESS;
}

void stringlist_init(stringlist_t *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void stringlist_free(stringlist_t *list) {
    if (list->items) {
        for (size_t i = 0; i < list->count; i++)
            free(list->items[i]);
        free((char *)(list->items));
    }
    stringlist_init(list);
}

/* intlist functions */
intlist_error_t intlist_append(intlist_t *list, int value) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 8;
        int *tmp = (int *)realloc(list->items, new_cap * sizeof(*tmp));
        if (tmp == NULL)
            return INTLIST_ERR_ALLOC_FAILED;
        list->items = tmp;
        list->capacity = new_cap;
    }
    list->items[list->count++] = value;
    return INTLIST_SUCCESS;
}

void intlist_init(intlist_t *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void intlist_free(intlist_t *list) {
    free(list->items);
    intlist_init(list);
}

intlist_t *intlist_new(void) {
    intlist_t *list = malloc(sizeof(intlist_t));
    if (list)
        intlist_init(list);
    return list;
}

bool in_intlist(int i, const intlist_t *list) {
    for (size_t j = 0; j < list->count; j++) {
        if (i == list->items[j]) {
            return true;
        }
    }
    return false;
}

void intlist_delete(intlist_t *list) {
    if (list) {
        intlist_free(list);
        free(list);
    }
}

void segmentlist_init(segmentlist_t *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}
void segmentlist_free(segmentlist_t *list) {
    free(list->items);
    segmentlist_init(list);
}

size_t segments_len(const segmentlist_t *segments) {
    if (segments == NULL) {
        return 0;
    }
    return segments->count;
}

segmentlist_error_t segmentlist_append(segmentlist_t *list, segment_t value) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 8;
        segment_t *tmp =
            (segment_t *)realloc(list->items, new_cap * sizeof(*tmp));
        if (tmp == NULL)
            return SEGMENTLIST_ERR_ALLOC_FAILED;
        list->items = tmp;
        list->capacity = new_cap;
    }
    list->items[list->count++] = value;
    return SEGMENTLIST_SUCCESS;
}

size_t get_segments(const char *text, const intlist_t *positions,
                    segmentlist_t *out) {
    size_t len = strlen(text);
    if (len == 0 || out == NULL)
        return 0;

    if (positions == NULL || positions->count == 0) {
        // TODO: Manage allocation error
        segmentlist_append(
            out, (segment_t){.highlighted = false, .start = 0, .end = len});
        return 1;
    }

    size_t start = 0;
    segment_type_t tmp = in_intlist(0, positions) ? HIGHLIGHTED : NORMAL;

    for (size_t i = 0; i < len; ++i) {
        segment_type_t type =
            in_intlist((int)i, positions) ? HIGHLIGHTED : NORMAL;

        if (type != tmp) {
            segment_t segment = {
                .highlighted = (tmp == HIGHLIGHTED), .start = start, .end = i};
            // TODO: Manage allocation error
            segmentlist_append(out, segment);
            start = i;
        }
        tmp = type;
    }

    segment_t last_segment = {
        .highlighted = (tmp == HIGHLIGHTED), .start = start, .end = len};
    segmentlist_append(out, last_segment);

    return out->count;
}
