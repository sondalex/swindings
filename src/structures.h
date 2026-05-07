#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    STRINGLIST_SUCCESS = 0,
    STRINGLIST_ERR_ALLOC_FAILED,

} stringlist_error_t;

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} stringlist_t;

stringlist_error_t stringlist_append(stringlist_t *list, const char *s);

void stringlist_init(stringlist_t *list);

void stringlist_free(stringlist_t *list);

typedef struct {
    int *items;
    size_t count;
    size_t capacity;
} intlist_t;

typedef enum {
    INTLIST_SUCCESS = 0,
    INTLIST_ERR_ALLOC_FAILED,
} intlist_error_t;

intlist_error_t intlist_append(intlist_t *list, int i);

bool in_intlist(int i, const intlist_t *list);

void intlist_init(intlist_t *list);

void intlist_free(intlist_t *list);

intlist_t *intlist_new(void);
void intlist_delete(intlist_t *list);

typedef struct {
    size_t start;
    size_t end;
    bool highlighted;
} segment_t;

typedef enum {
    NORMAL,
    HIGHLIGHTED,
} segment_type_t;

typedef struct {
    segment_t *items;
    size_t count;
    size_t capacity;
} segmentlist_t;

typedef enum {
    SEGMENTLIST_SUCCESS = 0,
    SEGMENTLIST_ERR_ALLOC_FAILED,
} segmentlist_error_t;

void segmentlist_init(segmentlist_t *list);
void segmentlist_free(segmentlist_t *list);

size_t get_segments(const char *text, const intlist_t *positions,
                    segmentlist_t *out);

size_t segments_len(const segmentlist_t *segments);
segmentlist_error_t segmentlist_append(segmentlist_t *list, segment_t value);
#endif
