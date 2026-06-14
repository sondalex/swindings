#include "config.h"
#include "asprintf.h"
#include "structures.h"
#include "utils.h"
#include <assert.h>
#include <ctype.h>
#include <glob.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void capitalize_into(const char *src, char *buf);

static void capitalize_into(const char *src, char *buf) {
    if (!src || !src[0]) {
        buf[0] = '\0';
        return;
    }
    size_t len = strlen(src);
    memcpy(buf, src, len);
    buf[len] = '\0';
    buf[0] = (char)toupper((unsigned char)buf[0]);
}

config_error_t config_read_file(const char *filepath, stringlist_t *out,
                                bool follow_includes) {
    if (filepath == NULL || out == NULL) {
        return CONFIG_ERR_INVALID_ARGUMENT;
    }
    FILE *fp = fopen(filepath, "r");
    if (!fp)
        return CONFIG_ERR_FILE_NOT_FOUND;

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

        if (follow_includes && strncmp(pos, "include ", 8) == 0) {
            char *pattern = pos + 8;
            while (*pattern == ' ' || *pattern == '\t')
                pattern++;
            glob_t globbuf;
            char *expanded = NULL;
            if (pattern[0] == '~') {
                const char *home = getenv("HOME");
                if (!home) {
                    return CONFIG_ERR_ENV_FAILED;
                }
                if (asprintf(&expanded, "%s%s", home, pattern + 1) < 0) {
                    return CONFIG_ERR_ALLOC_FAILED;
                }
                pattern = expanded;
            }
            int ret = glob(pattern, GLOB_NOCHECK, NULL, &globbuf);
            free(expanded);
            if (ret != 0 && ret != GLOB_NOMATCH) {
                globfree(&globbuf);
                free(line);
                int err = fclose(fp);
                if (err)
                    return CONFIG_ERR_IO;

                return CONFIG_ERR_GLOB_FAILED;
            }
            for (size_t gi = 0; gi < globbuf.gl_pathc; gi++) {
                /* Silently skip missing files from glob expansions */
                config_read_file(globbuf.gl_pathv[gi], out, follow_includes);
            }
            globfree(&globbuf);
            continue;
        }

        if (strncmp(pos, PATTERN, sizeof(PATTERN) - 1) == 0 &&
            pos[sizeof(PATTERN) - 1] == ' ') {
            if (stringlist_append(out, pos) != 0) {
                free(line);
                int err = fclose(fp);
                if (err)
                    return CONFIG_ERR_IO;
                return CONFIG_ERR_OUT_OF_MEMORY;
            }
        }
    }

    free(line);
    int err = fclose(fp);
    if (err)
        return CONFIG_ERR_IO;
    return CONFIG_SUCCESS;
}

// NOTE: Copied (with modifications) from
// https://github.com/swaywm/sway/blob/f1b40bc288f3be3bcc6a3c71f28ca9bb2529e70b/sway/config.c
static char *config_path(const char *prefix, const char *config_folder) {
    if (!prefix || !prefix[0] || !config_folder || !config_folder[0]) {
        return NULL;
    }
    char *path = NULL;
    // NOTE: Different here.
    if (asprintf(&path, "%s/%s/config", prefix, config_folder) < 0)
        return NULL;
    return path;
}

char *config_get_sway_filepath(void) {
    char *path = NULL;
    const char *home = getenv("HOME");
    const char *config_home = getenv("XDG_CONFIG_HOME");
    char *config_home_fallback = NULL;
    if (config_home == NULL || config_home[0] == '\0') {
        if (!home)
            return NULL;
        if (asprintf(&config_home_fallback, "%s/.config/", home) == -1)
            return NULL;
        config_home = config_home_fallback;
    }
    // NOTE: Copied from
    // https://github.com/swaywm/sway/blob/f1b40bc288f3be3bcc6a3c71f28ca9bb2529e70b/sway/config.c
    struct config_path {
        const char *prefix;
        const char *config_folder;
    };

    struct config_path config_paths[] = {
        {.prefix = home, .config_folder = ".sway"},
        {.prefix = config_home, .config_folder = "sway"},
        {.prefix = home, .config_folder = ".i3"},
        {.prefix = config_home, .config_folder = "i3"},
        {.prefix = SYSCONFDIR, .config_folder = "sway"},
        {.prefix = SYSCONFDIR, .config_folder = "i3"}};

    size_t num_config_paths = sizeof(config_paths) / sizeof(config_paths[0]);
    for (size_t i = 0; i < num_config_paths; i++) {
        path =
            config_path(config_paths[i].prefix, config_paths[i].config_folder);
        if (!path) {
            continue;
        }
        if (file_exists(path)) {
            break;
        }
        free(path);
        path = NULL;
    }

    free(config_home_fallback);
    return path;
}

void keymaplist_init(KeyMapList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

config_error_t keymaplist_append(KeyMapList *list, KeyMap km) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 8;
        KeyMap *tmp = realloc(list->items, new_cap * sizeof(*tmp));
        if (!tmp)
            return CONFIG_ERR_ALLOC_FAILED;
        list->items = tmp;
        list->capacity = new_cap;
    }
    list->items[list->count++] = km;
    return CONFIG_SUCCESS;
}

void keymap_free(KeyMap *km) {
    free(km->main_key);
    for (size_t i = 0; i < km->modifier_count; i++)
        free(km->modifiers[i]);
    free((char *)(km->modifiers));
    free(km->description);
}

void keymaplist_free(KeyMapList *list) {
    for (size_t i = 0; i < list->count; i++)
        keymap_free(&list->items[i]);
    free(list->items);
    keymaplist_init(list);
}
/**
 * Removes all flags and their values from a command string in-place.
 *
 * A flag is defined as a token starting with "--" followed immediately by
 * at least one alphabetic character. Two forms are supported:
 *   - Boolean flag:         --flag
 *   - Value flag (with =):  --flag=value
 *
 * The entire flag (including the value if the `=` form is used) is removed,
 * along with the single trailing space that follows it (if present).
 * The string is compacted in-place and properly null-terminated.
 *
 * @param string  The command string to modify. Must be a null-terminated,
 *                writable (mutable) buffer. Passing a string literal will
 *                cause a segmentation fault. If NULL, the function returns
 *                immediately without doing anything.
 *
 * @note
 * - Space-separated value flags of the form "--flag value" are **NOT**
 * supported.
 * - Flags must be separated by spaces from surrounding tokens.
 * - The function is safe against multiple consecutive flags and flags at the
 *   beginning or end of the string.
 * - The string buffer must be large enough to hold the original content
 *   (no extra space is allocated; the string only shrinks).
 *
 * @return void
 *
 * @example
 *   char cmd[] = "run --output=file.txt --verbose --debug=1 arg1 arg2";
 *   remove_flags(cmd);
 *   // cmd becomes: "run arg1 arg2"
 *
 *   char cmd2[] = "--help";
 *   remove_flags(cmd2);
 *   // cmd2 becomes: "" (empty string)
 *
 *   char cmd3[] = "ls --color=auto file.txt";
 *   remove_flags(cmd3);
 *   // cmd3 becomes: "ls file.txt"
 */
void remove_flags(char *string) {
    if (string == NULL) {
        return;
    }

    char *read = string;
    char *write = string;

    while (*read != '\0') {
        if (strncmp(read, "exec", 4) == 0 &&
            (read[4] == ' ' || read[4] == '\0' || read[4] == '\t')) {
            while (*read != '\0') {
                *write++ = *read++;
            }
            *write = '\0';
            return;
        }

        if (strncmp(read, "--", 2) == 0 && isalpha((char)read[2])) {
            read += 2;

            while (*read != '\0') {
                if (*read == '=') {
                    read++;
                    while (*read != ' ' && *read != '\0') {
                        read++;
                    }
                    break;
                }
                if (*read == ' ' || *read == '\0') {
                    break;
                }
                read++;
            }

            if (*read == ' ') {
                read++;
            }
            continue;
        }

        *write++ = *read++;
    }

    *write = '\0';
}

void normalize_space(char *string) {
    if (string == NULL)
        return;

    char *read = string;
    char *write = string;

    while (*read == ' ')
        read++;

    int in_space = 0;

    while (*read != '\0') {
        if (*read == ' ') {
            if (!in_space) {
                *write++ = ' ';
                in_space = 1;
            }
        } else {
            *write++ = *read;
            in_space = 0;
        }

        read++;
    }

    if (write > string && *(write - 1) == ' ')
        write--;

    *write = '\0';
}

config_error_t parse_key_maps(stringlist_t *lines, KeyMapList *out) {
    for (size_t i = 0; i < lines->count; i++) {
        char *key_combo = NULL;
        char *desc = NULL;
        char *tmp = NULL;
        char **tokens = NULL;
        size_t token_count = 0;

        char *cmd = strndup(lines->items[i], (size_t)(lines->items[i]));
        if (strncmp(cmd, "bindsym ", 8) != 0)
            continue;
        cmd += 8;
        normalize_space(cmd);
        remove_flags(cmd);
        char *space = strchr(cmd, ' ');
        if (!space)
            continue;

        size_t n = (size_t)(space - cmd);
        key_combo = strndup(cmd, n);
        if (!key_combo)
            goto fail;
        if (key_combo) {
            assert(strlen(key_combo));
        }

        size_t len = strlen(space + 1); // Length after the space
        desc = malloc(len + 1);
        if (!desc) {
            goto fail;
        }
        capitalize_into(space + 1, desc);
        if (!desc)
            goto fail;

        tmp = strdup(key_combo);
        if (!tmp)
            goto fail;

        char *tok = strtok(tmp, "+");
        while (tok) {
            char **new_tokens = (char **)(realloc(
                (char *)tokens, (token_count + 1) * sizeof(char *)));
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

        if (token_count == 0) {
            free(desc);
            free((char *)tokens);
            return CONFIG_ERR_ALLOC_FAILED; // or continue;
        }

        KeyMap km = {0};
        km.description = desc;
        km.main_key = tokens[token_count - 1];
        km.modifiers = tokens;
        km.modifier_count = token_count - 1;

        if (keymaplist_append(out, km) != 0) {
            keymap_free(&km);
            return CONFIG_ERR_ALLOC_FAILED;
        }
        continue;

    fail:
        for (size_t j = 0; j < token_count; j++)
            free(tokens[j]);
        free((char *)tokens);
        free(tmp);
        free(key_combo);
        free(desc);
        return CONFIG_ERR_ALLOC_FAILED;
    }
    return CONFIG_SUCCESS;
}
