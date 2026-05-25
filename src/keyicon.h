#ifndef KEYICON_H
#define KEYICON_H

#define MAX_CODEPOINTS 512

#include <stddef.h>
typedef struct {
    const char *name;
    const char *symbol;
    int codepoint;
} KeySymbol;



const char *key_to_symbol(const char *key);

extern const int key_codepoints[];
extern const int key_codepoints_count;

#endif
