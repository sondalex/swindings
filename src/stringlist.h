#ifndef STRINGLIST_H
#define STRINGLIST_H

#include <stddef.h>

typedef enum {
    STRINGLIST_SUCCESS = 0,
    STRINGLIST_ERR_ALLOC_FAILED,
    
} stringlist_error_t;


typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} StringList;

stringlist_error_t stringlist_append(StringList *list, const char *s);

void stringlist_init(StringList *list);


void stringlist_free(StringList *list);


#endif
