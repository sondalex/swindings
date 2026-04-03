#ifndef KEYICON_H
#define KEYICON_H

typedef struct {
    const char *name;
    const char *symbol;
    int codepoint;
} KeySymbol;



const char *key_to_symbol(const char *key);

extern const int key_codepoints[];
extern const int key_codepoints_count;

#endif
