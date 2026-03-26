#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "asprintf.h"



int stringlist_append(StringList *list, const char *s) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 8;
        char **tmp = (char **)realloc(list->items, new_cap * sizeof(*tmp));
        if (!tmp)
            return -1;
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
    stringlist_init(list); // reset to safe empty state
}

int read_file(const char *filepath, StringList *out) {
    FILE *fp = fopen(filepath, "r");
    if (!fp)
        return -1;

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while ((nread = getline(&line, &len, fp)) != -1) {
        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        char *pos = line;
        while (*pos == ' ' || *pos == '\t')
            pos++;
        if (*pos == '\0' || *pos == '#')
            continue;

        if (strncmp(pos, PATTERN, sizeof(PATTERN) - 1) == 0 &&
            pos[sizeof(PATTERN) - 1] == ' ') {
            if (stringlist_append(out, pos) != 0) {
                free(line);
                fclose(fp);
                return -1;
            }
        }
    }

    free(line);
    fclose(fp);
    return 0;
}

char *get_sway_config_filepath(void) {
    const char *home = getenv("HOME");
    if (!home)
        return NULL;

    char *path = NULL;
    if (asprintf(&path, "%s/.config/sway/config", home) == -1)
        return NULL;

    return path;
}

void keymaplist_init(KeyMapList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int keymaplist_append(KeyMapList *list, KeyMap km) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 8;
        KeyMap *tmp = realloc(list->items, new_cap * sizeof(*tmp));
        if (!tmp)
            return -1;
        list->items = tmp;
        list->capacity = new_cap;
    }
    list->items[list->count++] = km;
    return 0;
}

void keymap_free(KeyMap *km) {
    free(km->main_key);
    for (size_t i = 0; i < km->modifier_count; i++)
        free(km->modifiers[i]);
    free(km->modifiers);
    free(km->description);
}

void keymaplist_free(KeyMapList *list) {
    for (size_t i = 0; i < list->count; i++)
        keymap_free(&list->items[i]);
    free(list->items);
    keymaplist_init(list);
}

int parse_key_maps(StringList *lines, KeyMapList *out) {
    for (size_t i = 0; i < lines->count; i++) {
        char *key_combo = NULL;
        char *desc = NULL;
        char *tmp = NULL;
        char **tokens = NULL;
        size_t token_count = 0;

        char *pos = lines->items[i];
        if (strncmp(pos, "bindsym ", 8) != 0)
            continue;
        pos += 8;

        char *space = strchr(pos, ' ');
        if (!space)
            continue;

        key_combo = strndup(pos, space - pos);
        if (!key_combo)
            goto fail;

        desc = strdup(space + 1);
        if (!desc)
            goto fail;

        tmp = strdup(key_combo);
        if (!tmp)
            goto fail;

        char *tok = strtok(tmp, "+");
        while (tok) {
            char **new_tokens =
                realloc(tokens, (token_count + 1) * sizeof(char *));
            if (!new_tokens)
                goto fail;
            tokens = new_tokens;

            tokens[token_count] = strdup(tok);
            if (!tokens[token_count])
                goto fail;
            token_count++;

            tok = strtok(NULL, "+");
        }
        free(tmp);
        tmp = NULL;
        free(key_combo);
        key_combo = NULL;

        KeyMap km = {0};
        km.description = desc;
        km.main_key = tokens[token_count - 1];
        km.modifiers = tokens;
        km.modifier_count = token_count - 1;

        if (keymaplist_append(out, km) != 0) {
            keymap_free(&km);
            return -1;
        }
        continue;

    fail:
        for (size_t j = 0; j < token_count; j++)
            free(tokens[j]);
        free(tokens);
        free(tmp);
        free(key_combo);
        free(desc);
        return -1;
    }
    return 0;
}
