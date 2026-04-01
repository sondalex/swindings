#include "theme.h"
#include "tomlc17.h"
#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_ALPHA 180

static theme_error_t create_file(const char *filepath, bool create_dir);
static bool file_exists(const char *filepath);
static theme_color_t parse_hex_color(const char *hex_str);
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

static theme_color_t parse_hex_color(const char *hex_str) {
    theme_color_t c = {0, 0, 0, 255, false};

    if (hex_str == NULL)
        return c;

    if (hex_str[0] == '#')
        hex_str++;

    unsigned int r = 0, g = 0, b = 0, a = 255;

    if (sscanf(hex_str, "%02x%02x%02x%02x", &r, &g, &b, &a) == 4) {
        c.r = (uint8_t)r;
        c.g = (uint8_t)g;
        c.b = (uint8_t)b;
        c.a = (uint8_t)a;
        c.has_alpha = true;
        return c;
    }

    // Fall back to 6-digit format: #RRGGBB (alpha stays 255)
    if (sscanf(hex_str, "%02x%02x%02x", &r, &g, &b) == 3) {
        c.r = (uint8_t)r;
        c.g = (uint8_t)g;
        c.b = (uint8_t)b;
        return c;
    }

    return c;
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
    fclose(fd);
    return THEME_SUCCESS;
}

static void color_init_with_defaults(theme_color_t *color) {
    color->has_alpha = true;
    color->r = 20;
    color->g = 20;
    color->b = 20;
    color->a = DEFAULT_ALPHA;
}

static theme_color_t color_new_with_defaults(void) {
    theme_color_t color;
    color_init_with_defaults(&color);
    return color;
}

static void font_init_with_defaults(theme_font_t *font) {
    font->file = NULL;
    font->size = 14;
    font->color = (theme_color_t){
        .r = 130, .g = 130, .b = 130, .a = 255, .has_alpha = true};
}

static theme_font_t font_new_with_defaults(void) {
    theme_font_t font;
    font_init_with_defaults(&font);
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

    if (bg_color.type == TOML_STRING) {
        color = parse_hex_color(bg_color.u.str.ptr);
        layer->background.has_color = true;
    } else if (bg_color.type != TOML_UNKNOWN) {
        return THEME_PARSING_ERROR;
    }

    if (bg_alpha.type == TOML_FP64) {
        layer->background.alpha = (float)bg_alpha.u.fp64;
        layer->background.has_alpha = true;

        if (!color.has_alpha) {
            color.a = (uint8_t)(layer->background.alpha * 255.0f + 0.5f);
            color.has_alpha = true;
        }
    } else if (bg_alpha.type != TOML_UNKNOWN) {
        return THEME_PARSING_ERROR;
    }

    if (font_size.type == TOML_FP64) {
        font.size = (float)font_size.u.fp64;
        font.has_size = true;
    } else if (font_size.type != TOML_UNKNOWN) {
        return THEME_PARSING_ERROR;
    }

    if (font_file.type == TOML_STRING) {
        font.file = strdup(font_file.u.str.ptr);
        if (!font.file)
            return THEME_ALLOC_FAILED;
    } else if (font_file.type != TOML_UNKNOWN) {
        return THEME_PARSING_ERROR;
    }

    if (font_color.type == TOML_STRING) {
        font.color = parse_hex_color(font_color.u.str.ptr);
        font.has_color = true;
    } else if (font_color.type != TOML_UNKNOWN) {
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
    return theme_toml_parse(toml, layer, "theme.bottom.background.color",
                            "theme.bottom.background.alpha",
                            "theme.bottom.font.size", "theme.bottom.font.file",
                            "theme.bottom.font.color");
}

// Merge src into dest. Only fields explicitly set in src override dest.
static void theme_layer_merge(theme_layer_t *dest, const theme_layer_t *src) {
    if (!dest || !src)
        return;

    if (src->background.has_color)
        dest->background.color = src->background.color;
    if (src->background.has_alpha)
        dest->background.alpha = src->background.alpha;
    if (src->font.has_size)
        dest->font.size = src->font.size;

    // For strings: always duplicate (ownership transfer)
    if (src->font.file) {
        free(dest->font.file);
        dest->font.file = strdup(src->font.file);
    }

    if (src->font.has_color)
        dest->font.color = src->font.color;
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
    }

    if (success) {
        // Merge global into each specific layer (with proper string
        theme_layer_merge(&theme.top, &global_layer);
        theme_layer_merge(&theme.body, &global_layer);
        theme_layer_merge(&theme.bottom, &global_layer);

        res.theme = theme;
        res.error = THEME_SUCCESS;
    }

    theme_layer_free_strings(&global_layer);

    if (!success) {
        theme_layer_free_strings(&theme.top);
        theme_layer_free_strings(&theme.body);
        theme_layer_free_strings(&theme.bottom);
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
}

char *theme_get_config_filepath(void) {
    const char *home = getenv("HOME");
    if (!home)
        return NULL;

    const char *suffix = "/.config/swindings/config.toml";

    size_t len_home = strlen(home);
    int trim = (len_home > 0 && home[len_home - 1] == '/');

    size_t total_len = len_home - trim + strlen(suffix) + 1;

    char *path = malloc(total_len);
    if (!path)
        return NULL;

    snprintf(path, total_len, "%.*s%s", (int)(len_home - trim), home, suffix);

    return path;
}

theme_error_t theme_load_from_config(theme_t *theme) {
    char *filepath = theme_get_config_filepath();
    if (!filepath) {
        return THEME_CONFIG_NOT_FOUND;
    }

    if (!file_exists(filepath)) {
        theme_error_t err = create_file(filepath, true);

        if (err != THEME_SUCCESS) {
            free(filepath);
            return err;
        }
    }

    theme_result_t res = theme_load(filepath);
    free(filepath);

    if (res.error == THEME_SUCCESS) {
        *theme = res.theme;
        return THEME_SUCCESS;
    }

    return res.error;
}
