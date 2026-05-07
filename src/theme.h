#ifndef THEME_H
#define THEME_H

#include <stdbool.h>
#include <stdint.h>

#define THEME_DEFAULT_BG_ALPHA 180

#define DEFAULT_FONT_SUFFIX "share/fonts/JetBrainsMonoNerdFont-Regular.ttf"
#define F_DEFAULT_FONT_SUFFIX "%s/" DEFAULT_FONT_SUFFIX 

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;      // always valid (default 255)
    bool has_alpha; // true = user specified alpha in TOML
} theme_color_t;

typedef struct {
    theme_color_t color;
    float alpha;    // 0.0–1.0, only meaningful if has_alpha == true or global
    bool has_color; // true = explicitly set in TOML
    bool has_alpha; // true = explicitly set in TOML
} theme_background_t;

typedef struct {
    float size; // > 0.0 = specified, else inherit
    char *file; // NULL = no custom file (use family or system)
    theme_color_t color;
    bool has_size;  // true = explicitly set in TOML
    bool has_color; // true = explicitly set in TOML
} theme_font_t;

typedef struct {
    theme_background_t background;
    theme_font_t font;
    // add border, padding, etc. later
} theme_layer_t;

typedef struct {
    theme_layer_t top;
    theme_layer_t body;
    theme_layer_t bottom;
    theme_layer_t highlight;
} theme_t;

typedef enum {
    THEME_SUCCESS,
    THEME_IO_ERROR,
    THEME_ALLOC_FAILED,
    THEME_PARSING_ERROR,
    THEME_CONFIG_NOT_FOUND,
} theme_error_t;

typedef struct {
    theme_t theme;
    theme_error_t error;
} theme_result_t;

void parse_hex_color(const char *hex_str, theme_color_t *c);

/**
 * Returns the full path to the swindings configuration file.
 *
 * Memory:
 *   - The returned string is dynamically allocated using malloc.
 *   - The caller is responsible for freeing it using free().
 *
 * Returns:
 *   - A pointer to a newly allocated null-terminated string on success.
 *   - NULL if the HOME environment variable is not set or if allocation
 * fails.
 */
char *theme_get_config_filepath(void);
const char *theme_error_str(theme_error_t err);
theme_result_t theme_load(const char *filepath);
theme_error_t theme_load_from_filepath(theme_t *theme, const char *filepath);
theme_error_t theme_load_from_config(theme_t *theme);
void theme_free(theme_t *theme);

#endif
