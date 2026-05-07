#include "structures.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CANDIDATE_LEN 1024


typedef enum {
    LITERAL_SEARCH = 0,
    FUZZY_SEARCH,
} search_type_t;

typedef struct {
    bool active;
    char *text;
    size_t max_length;
    size_t text_length;
    search_type_t type;
    bool query_changed;
} search_state_t;

typedef struct {
    // Mask of activated field
    size_t count;
    bool *mask;
    // Two D array. One array contains the position of the character matches in
    // a string
    intlist_t **positions;
} search_result_t;

int init_search_result(search_result_t *result, size_t count);
void free_search_result(search_result_t *result);
void update_search_input(search_state_t *state);
void init_search_state(search_state_t *state, char *buffer, size_t buffer_size);
void search(const char *query, const char *candidates[], search_type_t type,
            search_result_t *out);
