#include "theme.h"
#include "asprintf.h"
#include "tomlc17.h"
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static theme_error_t create_file(const char *filepath, bool create_dir);
static bool file_exists(const char *filepath);
static void color_init_with_defaults(theme_color_t *color);
static theme_color_t color_new_with_defaults(void);
static void font_init_with_defaults(theme_font_t *font);
static theme_font_t font_new_with_defaults(void);
static void theme_layer_free_strings(theme_layer_t *layer);
static theme_error_t
theme_toml_parse(const toml_result_t *toml, theme_layer_t *layer,
                 const char *bg_color_key, const char *bg_alpha_key,
                 const char *font_size_key, const char *font_file_key,
                 const char *font_color_key);
static theme_error_t theme_toml_parse_global(const toml_result_t *toml,
                                             theme_layer_t *layer);
static theme_error_t theme_toml_parse_top(const toml_result_t *toml,
                                          theme_layer_t *layer);
static theme_error_t theme_toml_parse_body(const toml_result_t *toml,
                                           theme_layer_t *layer);
static theme_error_t theme_toml_parse_bottom(const toml_result_t *toml,
                                             theme_layer_t *layer);
static void theme_layer_merge(theme_layer_t *dest, const theme_layer_t *src);

static bool file_exists(const char *filepath) {
    return (bool)(access(filepath, F_OK) == 0);
}

void parse_hex_color(const char *hex_str, theme_color_t *c) {
    if (hex_str == NULL || c == NULL)
        return;

    if (hex_str[0] != '#')
        return;

    hex_str++; // skip '#'

    size_t len = strlen(hex_str);
    if (len != 6 && len != 8)
        return;

    unsigned long r = 0, g = 0, b = 0, a = 255;
    char *endptr;
    bool success = false;

    errno = 0;

    // Try 8-digit format: #RRGGBBAA
    if (len == 8) {
        r = strtoul(hex_str, &endptr, 16);
        if (endptr != hex_str + 2)
            goto fail;

        g = strtoul(endptr, &endptr, 16);
        if (endptr != hex_str + 4)
            goto fail;

        b = strtoul(endptr, &endptr, 16);
        if (endptr != hex_str + 6)
            goto fail;

        a = strtoul(endptr, &endptr, 16);
        if (endptr != hex_str + 8 || *endptr != '\0')
            goto fail;

        c->has_alpha = true;
        success = true;
    }
    // Try 6-digit format: #RRGGBB
    else if (len == 6) {
        r = strtoul(hex_str, &endptr, 16);
        if (endptr != hex_str + 2)
            goto fail;

        g = strtoul(endptr, &endptr, 16);
        if (endptr != hex_str + 4)
            goto fail;

        b = strtoul(endptr, &endptr, 16);
        if (endptr != hex_str + 6 || *endptr != '\0')
            goto fail;

        c->has_alpha = false;
        success = true;
    }

fail:
    if (success && errno == 0) {
        c->r = (uint8_t)r;
        c->g = (uint8_t)g;
        c->b = (uint8_t)b;
        c->a = (uint8_t)a;
    }
}

static char *get_directory(const char *filepath) {
    if (!filepath || !*filepath) {
        return NULL;
    }
    char *copy = strdup(filepath);
    if (!copy)
        return NULL;
    dirname(copy);
    return copy;
}

static theme_error_t make_dir(const char *dir) {
    mode_t old_mode = umask(0);
    if (mkdir(dir, 0755) != 0) {
        if (errno != EEXIST) { // "already exists" is usually OK
            umask(old_mode);
            return THEME_IO_ERROR;
        }
    }
    umask(old_mode);
    return THEME_SUCCESS;
}

static theme_error_t create_file(const char *filepath, bool create_dir) {
    if (!filepath || !*filepath) {
        return THEME_IO_ERROR;
    }

    if (create_dir) {
        char *dir = get_directory(filepath);
        if (!dir) {
            return THEME_IO_ERROR;
        }

        if (!file_exists(dir)) {
            theme_error_t err = make_dir(dir);
            free(dir);
            if (err != THEME_SUCCESS) {
                return err;
            }
        } else {
            free(dir);
        }
    }

    FILE *fd = fopen(filepath, "w");
    if (!fd) {
        return THEME_IO_ERROR;
    }
    int err = fclose(fd);
    if (err) {
        return THEME_IO_ERROR;
    }
    return THEME_SUCCESS;
}

static void color_init_with_defaults(theme_color_t *color) {
    color->has_alpha = true;
    color->r = 20;
    color->g = 20;
    color->b = 20;
    color->a = THEME_DEFAULT_BG_ALPHA;
}

static theme_color_t color_new_with_defaults(void) {
    theme_color_t color;
    color_init_with_defaults(&color);
    return color;
}

static void font_init_with_defaults(theme_font_t *font) {
    font->file = NULL;
    font->size = 16;
    font->has_size = false;
    font->has_color = false;
    font->color = (theme_color_t){
        .r = 130, .g = 130, .b = 130, .a = 255, .has_alpha = true};
}

static char *get_app_prefix(void) {
    char exe_buf[PATH_MAX + 1];
    ssize_t len = readlink("/proc/self/exe", exe_buf, PATH_MAX);
    if (len == -1)
        return NULL;

    exe_buf[len] = '\0';

    char *last_slash = strrchr(exe_buf, '/');
    if (last_slash)
        *last_slash = '\0';

    last_slash = strrchr(exe_buf, '/');
    if (last_slash)
        *last_slash = '\0';

    return strdup(exe_buf);
}

static char *resolve_default_font_path(void) {
    char *prefix = get_app_prefix();
    if (!prefix)
        return NULL;

    char *font_path = NULL;
    asprintf(&font_path, F_DEFAULT_FONT_SUFFIX, prefix);

    free(prefix);
    return font_path;
}

static theme_font_t font_new_with_defaults(void) {
    theme_font_t font;
    font_init_with_defaults(&font);
    font.file = resolve_default_font_path();
    return font;
}

static theme_layer_t layer_new_with_defaults(void) {
    return (theme_layer_t){
        .background = {.color = color_new_with_defaults()},
        .font = font_new_with_defaults(),
    };
}

static theme_layer_t layer_new_with_top_defaults(void) {
    theme_layer_t l = layer_new_with_defaults();
    l.font.color = (theme_color_t){
        .r = 160, .g = 160, .b = 160, .a = 255, .has_alpha = true};
    return l;
}

static void theme_layer_free_strings(theme_layer_t *layer) {
    if (!layer)
        return;
    if (layer && layer->font.file) {
        free(layer->font.file);
        layer->font.file = NULL;
    }
}

static theme_error_t
theme_toml_parse(const toml_result_t *toml, theme_layer_t *layer,
                 const char *bg_color_key, const char *bg_alpha_key,
                 const char *font_size_key, const char *font_file_key,
                 const char *font_color_key) {
    if (!layer)
        return THEME_PARSING_ERROR;

    toml_datum_t bg_color = toml_seek(toml->toptab, bg_color_key);
    toml_datum_t bg_alpha = toml_seek(toml->toptab, bg_alpha_key);
    toml_datum_t font_size = toml_seek(toml->toptab, font_size_key);
    toml_datum_t font_file = toml_seek(toml->toptab, font_file_key);
    toml_datum_t font_color = toml_seek(toml->toptab, font_color_key);

    theme_color_t color = layer->background.color;
    theme_font_t font = layer->font;

    // Parse background color
    if (bg_color.type == TOML_STRING) {
        parse_hex_color(bg_color.u.str.ptr, &color);
        layer->background.has_color = true;
    } else if (bg_color.type != TOML_UNKNOWN) {
        return THEME_PARSING_ERROR;
    }

    // Parse background alpha
    if (bg_alpha.type == TOML_FP64) {
        layer->background.alpha = (float)bg_alpha.u.fp64;
        layer->background.has_alpha = true;
        color.a = (uint8_t)(layer->background.alpha * 255.0f + 0.5f);
        color.has_alpha = true;
    } else if (bg_alpha.type == TOML_INT64) {
        layer->background.alpha = (float)bg_alpha.u.int64;
        layer->background.has_alpha = true;
        color.a = (uint8_t)(layer->background.alpha * 255.0f + 0.5f);
        color.has_alpha = true;
    } else if (bg_alpha.type != TOML_UNKNOWN) {
        return THEME_PARSING_ERROR;
    }

    // Parse font size
    if (font_size.type == TOML_FP64) {
        font.size = (float)font_size.u.fp64;
        font.has_size = true;
    } else if (font_size.type == TOML_INT64) {
        font.size = (float)font_size.u.int64;
        font.has_size = true;
    } else if (font_size.type != TOML_UNKNOWN) {
        return THEME_PARSING_ERROR;
    }

    // Parse font file
    if (font_file.type == TOML_STRING) {
        free(font.file);
        if (strlen(font_file.u.str.ptr) > 0) {
            font.file = strdup(font_file.u.str.ptr);
            if (!font.file)
                return THEME_ALLOC_FAILED;
        } else {
            font.file = NULL;
        }
    } else if (font_file.type != TOML_UNKNOWN) {
        return THEME_PARSING_ERROR;
    }

    // Parse font color
    if (font_color.type == TOML_STRING) {
        parse_hex_color(font_color.u.str.ptr, &font.color);
        font.has_color = true;
    } else if (font_color.type != TOML_UNKNOWN) {
        if (font.file != NULL) {
            free(font.file);
        }
        return THEME_PARSING_ERROR;
    }

    layer->background.color = color;
    layer->font = font;

    return THEME_SUCCESS;
}

static theme_error_t theme_toml_parse_global(const toml_result_t *toml,
                                             theme_layer_t *layer) {
    *layer = layer_new_with_defaults();
    return theme_toml_parse(toml, layer, "theme.background.color",
                            "theme.background.alpha", "theme.font.size",
                            "theme.font.file", "theme.font.color");
}
static theme_error_t theme_toml_parse_top(const toml_result_t *toml,
                                          theme_layer_t *layer) {
    *layer = layer_new_with_top_defaults();
    return theme_toml_parse(toml, layer, "theme.top.background.color",
                            "theme.top.background.alpha", "theme.top.font.size",
                            "theme.top.font.file", "theme.top.font.color");
}
static theme_error_t theme_toml_parse_body(const toml_result_t *toml,
                                           theme_layer_t *layer) {
    *layer = layer_new_with_defaults();
    return theme_toml_parse(toml, layer, "theme.body.background.color",
                            "theme.body.background.alpha",
                            "theme.body.font.size", "theme.body.font.file",
                            "theme.body.font.color");
}

static theme_error_t theme_toml_parse_bottom(const toml_result_t *toml,
                                             theme_layer_t *layer) {
    *layer = layer_new_with_defaults();
    layer->font.size = layer->font.size + 3;
    return theme_toml_parse(toml, layer, "theme.bottom.background.color",
                            "theme.bottom.background.alpha",
                            "theme.bottom.font.size", "theme.bottom.font.file",
                            "theme.bottom.font.color");
}

static theme_error_t theme_toml_parse_highlight(const toml_result_t *toml,
                                                const theme_layer_t *base,
                                                theme_layer_t *layer) {
    *layer = *base;
    if (base->font.file) {
        layer->font.file = strdup(base->font.file);
    }

    if (!layer->background.has_color) {
        layer->background.color = (theme_color_t){
            .r = 100, .g = 180, .b = 255, .a = 128, .has_alpha = true};
    }

    const char *bg_color_key = "theme.highlight.background.color";
    const char *bg_alpha_key = "theme.highlight.background.alpha";
    const char *font_size_key = "theme.highlight.font.size";
    const char *font_file_key = "theme.highlight.font.file";
    const char *font_color_key = "theme.highlight.font.color";

    return theme_toml_parse(toml, layer, bg_color_key, bg_alpha_key,
                            font_size_key, font_file_key, font_color_key);
}

static void theme_layer_merge(theme_layer_t *dest, const theme_layer_t *src) {
    if (!dest || !src)
        return;

    // Background color / alpha / font size (value types - safe to copy)
    if (!dest->background.has_color && src->background.has_color)
        dest->background.color = src->background.color;

    if (!dest->background.has_alpha && src->background.has_alpha) {
        dest->background.alpha = src->background.alpha;
        dest->background.color.a =
            (uint8_t)(dest->background.alpha * 255.0f + 0.5f);
        dest->background.has_alpha = true;
    }

    if (!dest->font.has_size && src->font.has_size) {
        dest->font.size = src->font.size;
        dest->font.has_size = true;
    }

    if (!dest->font.has_color && src->font.has_color)
        dest->font.color = src->font.color;

    // === String handling (ownership) ===
    // Only override if dest doesn't have a font file yet
    if (src->font.file && !dest->font.file) {
        dest->font.file = strdup(src->font.file);
        // TODO: handle allocation failure gracefully if needed
    }
}

const char *theme_error_str(theme_error_t err) {
    switch (err) {
    case THEME_SUCCESS:
        return "success";
    case THEME_IO_ERROR:
        return "could not read/write config file";
    case THEME_ALLOC_FAILED:
        return "memory allocation failed";
    case THEME_PARSING_ERROR:
        return "failed to parse config TOML";
    case THEME_CONFIG_NOT_FOUND:
        return "return file path could not be determined (HOME not set?)";
    }
    return "unknown error";
}

theme_result_t theme_load(const char *filepath) {
    theme_result_t res = {.theme = {0}, .error = THEME_PARSING_ERROR};

    toml_result_t toml = toml_parse_file_ex(filepath);
    if (!toml.ok) {
        return res;
    }

    theme_layer_t global_layer = {0};
    theme_t theme = {0};

    bool success = true;

    if (theme_toml_parse_global(&toml, &global_layer) != THEME_SUCCESS) {
        success = false;
    }

    if (success) {
        if (theme_toml_parse_top(&toml, &theme.top) != THEME_SUCCESS ||
            theme_toml_parse_body(&toml, &theme.body) != THEME_SUCCESS ||
            theme_toml_parse_bottom(&toml, &theme.bottom) != THEME_SUCCESS) {
            success = false;
        }

        // Parse highlight AFTER body
        if (success) {
            if (theme_toml_parse_highlight(&toml, &theme.body,
                                           &theme.highlight) != THEME_SUCCESS) {
                success = false;
            }
        }
    }

    if (success) {
        // Merge global into each specific layer (with proper string
        theme_layer_merge(&theme.top, &global_layer);
        theme_layer_merge(&theme.body, &global_layer);
        theme_layer_merge(&theme.bottom, &global_layer);
        theme_layer_merge(&theme.highlight, &theme.body);

        res.theme = theme;
        res.error = THEME_SUCCESS;
    }

    theme_layer_free_strings(&global_layer);

    if (!success) {
        theme_layer_free_strings(&theme.top);
        theme_layer_free_strings(&theme.body);
        theme_layer_free_strings(&theme.bottom);
        theme_layer_free_strings(&theme.highlight);
    }

    toml_free(toml);
    return res;
}

void theme_free(theme_t *theme) {
    if (!theme)
        return;

    theme_layer_free_strings(&theme->top);
    theme_layer_free_strings(&theme->body);
    theme_layer_free_strings(&theme->bottom);
    theme_layer_free_strings(&theme->highlight);
}

char *theme_get_config_filepath(void) {
    const char *home = getenv("HOME");
    if (!home)
        return NULL;

    const char *suffix = "/.config/swindings/config.toml";

    size_t len_home = strlen(home);
    int trim = (len_home > 0 && home[len_home - 1] == '/');

    size_t total_len = len_home - (size_t)trim + strlen(suffix) + 1;

    char *path = malloc(total_len);
    if (!path)
        return NULL;

    int count = snprintf(path, total_len, "%.*s%s",
                         (int)(len_home - (size_t)trim), home, suffix);
    if (count < 0) {
        free(path);
        return NULL;
    }

    return path;
}

theme_error_t theme_load_from_filepath(theme_t *theme, const char *filepath) {
    if (!filepath) {
        return THEME_CONFIG_NOT_FOUND;
    }

    if (!file_exists(filepath)) {
        theme_error_t err = create_file(filepath, true);

        if (err != THEME_SUCCESS) {
            return err;
        }
    }

    theme_result_t res = theme_load(filepath);

    if (res.error == THEME_SUCCESS) {
        *theme = res.theme;
        return THEME_SUCCESS;
    }

    return res.error;
}

theme_error_t theme_load_from_config(theme_t *theme) {
    char *filepath = theme_get_config_filepath();
    theme_error_t res = theme_load_from_filepath(theme, filepath);
    free(filepath);
    return res;
}
