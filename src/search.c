#include "search.h"
#include "match.h"
#include "raylib.h"
#include <stdlib.h>
#include <string.h>

void init_search_state(search_state_t *state, char *buffer,
                       size_t buffer_size) {
    if (!state || !buffer || buffer_size == 0)
        return;

    state->active = false;
    state->text = buffer;
    state->max_length = buffer_size;
    state->text_length = 0;
    state->type = LITERAL_SEARCH;
    state->text[0] = '\0';
    state->query_changed = false;
}

void update_search_input(search_state_t *state) {
    if (!state || !state->text)
        return;

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_F)) {
            state->active = true;
            state->type = LITERAL_SEARCH;
            state->query_changed = true;
        } else if (IsKeyPressed(KEY_K)) {
            state->active = true;
            state->type = FUZZY_SEARCH;
            state->query_changed = true;
        }
    }

    if (!state->active)
        return;

    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) &&
            (state->text_length < state->max_length - 1)) {
            state->text[state->text_length] = (char)key;
            state->text_length++;
            state->text[state->text_length] = '\0';
            state->query_changed = true;
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && state->text_length > 0) {
        state->text_length--;
        state->text[state->text_length] = '\0';
        state->query_changed = true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        state->active = false;
        state->query_changed = true; /* important: force refresh when closing */
    }
}

int init_search_result(search_result_t *result, size_t count) {
    if (result == NULL || count == 0)
        return -1;

    memset(result, 0, sizeof(*result));
    result->count = count;

    result->mask = (bool *)calloc(count, sizeof(*result->mask));
    if (result->mask == NULL)
        return -1;

    result->positions = (intlist_t **)calloc(count, sizeof(*result->positions));
    if (result->positions == NULL) {
        free(result->mask);
        result->mask = NULL;
        return -1;
    }

    return 0;
}

void free_search_result(search_result_t *result) {
    if (!result)
        return;

    if (result->positions) {
        for (size_t i = 0; i < result->count; i++) {
            if (result->positions[i]) {
                intlist_delete(result->positions[i]); /* uses new helper */
            }
        }
        free(result->positions);
    }
    free(result->mask);
    memset(result, 0, sizeof(*result));
}

static void clear_search_result(search_result_t *result) {
    if (!result)
        return;
    for (size_t i = 0; i < result->count; i++) {
        if (result->positions[i]) {
            intlist_delete(result->positions[i]);
            result->positions[i] = NULL;
        }
    }
    if (result->mask)
        memset(result->mask, 0, result->count * sizeof(bool));
}

void search(const char *query, const char *candidates[], search_type_t type,
            search_result_t *out) {
    if (!out || !candidates)
        return;

    /* Clear previous results first */
    clear_search_result(out);

    if (!query || query[0] == '\0')
        return; /* empty query → everything stays false (handled by caller) */

    for (size_t i = 0; candidates[i] != NULL; i++) {
        if (i >= out->count)
            break;

        const char *candidate = candidates[i];
        intlist_t **ptr_candidate_position = &out->positions[i];

        if (type == LITERAL_SEARCH) {
            const char *ptr = candidate;
            bool has_match = false;

            while ((ptr = strstr(ptr, query)) != NULL) {
                has_match = true;
                size_t start = (size_t)(ptr - candidate);
                size_t len = strlen(query);

                if (*ptr_candidate_position == NULL) {
                    *ptr_candidate_position = intlist_new();
                    if (*ptr_candidate_position == NULL)
                        break;
                }

                for (size_t j = 0; j < len; j++) {
                    intlist_append(*ptr_candidate_position, (int)(start + j));
                }

                ptr += len; /* move past this match */
            }

            out->mask[i] = has_match;
        } else {
            if (has_match(query, candidate)) {
                if (*ptr_candidate_position == NULL) {
                    *ptr_candidate_position = intlist_new();
                    if (*ptr_candidate_position == NULL)
                        continue;
                    out->positions[i] = *ptr_candidate_position;
                }
                size_t positions[MAX_CANDIDATE_LEN];
                size_t npos = 0;
                match_positions(query, candidate, positions, &npos);
                for (size_t j = 0; j < npos; j++)
                    intlist_append(*ptr_candidate_position, (int)positions[j]);
                out->mask[i] = true;
            } else {
                out->mask[i] = false;
            }
        }
    }
}
