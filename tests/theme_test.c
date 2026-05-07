#include "theme.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int ends_with(const char *str, const char *suffix) {
    if (!str || !suffix)
        return 0;
    size_t len_str = strlen(str);
    size_t len_suf = strlen(suffix);
    if (len_suf > len_str)
        return 0;
    return strcmp(str + len_str - len_suf, suffix) == 0;
}

static char *create_temp_toml(const char *content) {
    char template[] = "/tmp/theme_test_XXXXXX";
    int fd = mkstemp(template);
    if (fd == -1)
        return NULL;
    close(fd);

    FILE *f = fopen(template, "w");
    if (!f)
        return NULL;
    fputs(content, f);
    fclose(f);

    return strdup(template);
}

static void delete_temp_file(char *path) {
    if (path) {
        remove(path);
        free(path);
    }
}

void test_theme_load_nonexistent_file_creates_it_and_loads_defaults(void) {
    theme_t theme;
    char path[32];
    snprintf(path, sizeof(path), "/tmp/file_%d_%ld", getpid(), (long)rand());

    theme_error_t res = theme_load_from_filepath(&theme, path);
    TEST_ASSERT_EQUAL(THEME_SUCCESS, res);
    TEST_ASSERT_TRUE(access(path, F_OK) == 0);

    TEST_ASSERT_EQUAL(20, theme.top.background.color.r);
    TEST_ASSERT_EQUAL(20, theme.top.background.color.g);
    TEST_ASSERT_EQUAL(20, theme.top.background.color.b);
    TEST_ASSERT_EQUAL(180, theme.top.background.color.a);
    TEST_ASSERT_EQUAL(160, theme.top.font.color.r);
    TEST_ASSERT_EQUAL(160, theme.top.font.color.g);
    TEST_ASSERT_EQUAL(160, theme.top.font.color.b);
    TEST_ASSERT_EQUAL(255, theme.top.font.color.a);
    TEST_ASSERT_EQUAL(true, theme.top.font.color.has_alpha);
    TEST_ASSERT_EQUAL(16, theme.top.font.size);
    TEST_ASSERT_TRUE(ends_with(theme.top.font.file, DEFAULT_FONT_SUFFIX));

    TEST_ASSERT_EQUAL(20, theme.body.background.color.r);
    TEST_ASSERT_EQUAL(20, theme.body.background.color.g);
    TEST_ASSERT_EQUAL(20, theme.body.background.color.b);
    TEST_ASSERT_EQUAL(180, theme.body.background.color.a);
    TEST_ASSERT_EQUAL(130, theme.body.font.color.r);
    TEST_ASSERT_EQUAL(130, theme.body.font.color.g);
    TEST_ASSERT_EQUAL(130, theme.body.font.color.b);
    TEST_ASSERT_EQUAL(255, theme.body.font.color.a);
    TEST_ASSERT_EQUAL(true, theme.body.font.color.has_alpha);
    TEST_ASSERT_EQUAL(16, theme.body.font.size);
    TEST_ASSERT_TRUE(ends_with(theme.body.font.file, DEFAULT_FONT_SUFFIX));

    TEST_ASSERT_EQUAL(20, theme.bottom.background.color.r);
    TEST_ASSERT_EQUAL(20, theme.bottom.background.color.g);
    TEST_ASSERT_EQUAL(20, theme.bottom.background.color.b);
    TEST_ASSERT_EQUAL(180, theme.bottom.background.color.a);
    TEST_ASSERT_EQUAL(130, theme.bottom.font.color.r);
    TEST_ASSERT_EQUAL(130, theme.bottom.font.color.g);
    TEST_ASSERT_EQUAL(130, theme.bottom.font.color.b);
    TEST_ASSERT_EQUAL(255, theme.bottom.font.color.a);
    TEST_ASSERT_EQUAL(true, theme.bottom.font.color.has_alpha);
    TEST_ASSERT_EQUAL(19, theme.bottom.font.size);
    TEST_ASSERT_TRUE(ends_with(theme.bottom.font.file, DEFAULT_FONT_SUFFIX));

    TEST_ASSERT_EQUAL(100, theme.highlight.background.color.r);
    TEST_ASSERT_EQUAL(180, theme.highlight.background.color.g);
    TEST_ASSERT_EQUAL(255, theme.highlight.background.color.b);
    TEST_ASSERT_EQUAL(128, theme.highlight.background.color.a);

    TEST_ASSERT_EQUAL(130, theme.highlight.font.color.r);
    TEST_ASSERT_EQUAL(130, theme.highlight.font.color.g);
    TEST_ASSERT_EQUAL(130, theme.highlight.font.color.b);
    TEST_ASSERT_EQUAL(255, theme.highlight.font.color.a);
    TEST_ASSERT_TRUE(theme.highlight.font.color.has_alpha);

    TEST_ASSERT_EQUAL(16, theme.highlight.font.size);
    TEST_ASSERT_TRUE(ends_with(theme.highlight.font.file, DEFAULT_FONT_SUFFIX));
}

void test_theme_load_empty_file_returns_all_defaults(void) {
    char *path = create_temp_toml("");
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    TEST_ASSERT_EQUAL(THEME_SUCCESS, res.error);
    TEST_ASSERT_EQUAL(20, res.theme.top.background.color.r);
    TEST_ASSERT_EQUAL(20, res.theme.top.background.color.g);
    TEST_ASSERT_EQUAL(20, res.theme.top.background.color.b);
    TEST_ASSERT_EQUAL(180, res.theme.top.background.color.a);
    TEST_ASSERT_EQUAL(160, res.theme.top.font.color.r);
    TEST_ASSERT_EQUAL(160, res.theme.top.font.color.g);
    TEST_ASSERT_EQUAL(160, res.theme.top.font.color.b);
    TEST_ASSERT_EQUAL(255, res.theme.top.font.color.a);
    TEST_ASSERT_EQUAL(true, res.theme.top.font.color.has_alpha);
    TEST_ASSERT_EQUAL(16, res.theme.top.font.size);
    TEST_ASSERT_TRUE(ends_with(res.theme.top.font.file, DEFAULT_FONT_SUFFIX));

    TEST_ASSERT_EQUAL(20, res.theme.body.background.color.r);
    TEST_ASSERT_EQUAL(20, res.theme.body.background.color.g);
    TEST_ASSERT_EQUAL(20, res.theme.body.background.color.b);
    TEST_ASSERT_EQUAL(180, res.theme.body.background.color.a);
    TEST_ASSERT_EQUAL(130, res.theme.body.font.color.r);
    TEST_ASSERT_EQUAL(130, res.theme.body.font.color.g);
    TEST_ASSERT_EQUAL(130, res.theme.body.font.color.b);
    TEST_ASSERT_EQUAL(255, res.theme.body.font.color.a);
    TEST_ASSERT_EQUAL(true, res.theme.body.font.color.has_alpha);
    TEST_ASSERT_EQUAL(16, res.theme.body.font.size);
    TEST_ASSERT_TRUE(ends_with(res.theme.body.font.file, DEFAULT_FONT_SUFFIX));

    TEST_ASSERT_EQUAL(20, res.theme.bottom.background.color.r);
    TEST_ASSERT_EQUAL(20, res.theme.bottom.background.color.g);
    TEST_ASSERT_EQUAL(20, res.theme.bottom.background.color.b);
    TEST_ASSERT_EQUAL(180, res.theme.bottom.background.color.a);
    TEST_ASSERT_EQUAL(130, res.theme.bottom.font.color.r);
    TEST_ASSERT_EQUAL(130, res.theme.bottom.font.color.g);
    TEST_ASSERT_EQUAL(130, res.theme.bottom.font.color.b);
    TEST_ASSERT_EQUAL(255, res.theme.bottom.font.color.a);
    TEST_ASSERT_EQUAL(true, res.theme.bottom.font.color.has_alpha);
    TEST_ASSERT_EQUAL(19, res.theme.bottom.font.size);
    TEST_ASSERT_TRUE(
        ends_with(res.theme.bottom.font.file, DEFAULT_FONT_SUFFIX));

    TEST_ASSERT_EQUAL(100, res.theme.highlight.background.color.r);
    TEST_ASSERT_EQUAL(180, res.theme.highlight.background.color.g);
    TEST_ASSERT_EQUAL(255, res.theme.highlight.background.color.b);
    TEST_ASSERT_EQUAL(128, res.theme.highlight.background.color.a);

    TEST_ASSERT_EQUAL(130, res.theme.highlight.font.color.r);
    TEST_ASSERT_EQUAL(130, res.theme.highlight.font.color.g);
    TEST_ASSERT_EQUAL(130, res.theme.highlight.font.color.b);
    TEST_ASSERT_EQUAL(255, res.theme.highlight.font.color.a);
    TEST_ASSERT_TRUE(res.theme.highlight.font.color.has_alpha);

    TEST_ASSERT_EQUAL(16, res.theme.highlight.font.size);
    TEST_ASSERT_TRUE(
        ends_with(res.theme.highlight.font.file, DEFAULT_FONT_SUFFIX));
}

void test_global_overrides_section_if_not_set(void) {
    char *path = create_temp_toml("theme.background.color = \"#112233\"\n"
                                  "theme.font.size=16\n"
                                  "[theme.top]\n"
                                  "font.size = 20\n");
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    TEST_ASSERT_EQUAL(
        20, res.theme.top.font.size); // Should NOT be overridden by global
    TEST_ASSERT_EQUAL(0x11, res.theme.top.background.color.r);
}

void test_section_overrides_global(void) {
    char *path = create_temp_toml("theme.background.color = \"#112233\"\n"
                                  "[theme.top]\n"
                                  "background.color = \"#aabbcc\"\n");
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    TEST_ASSERT_EQUAL(0xaa, res.theme.top.background.color.r);
    TEST_ASSERT_NOT_EQUAL(0x11, res.theme.top.background.color.r);
}

void test_global_alpha_propagates_to_sections(void) {
    char *path = create_temp_toml("theme.background.alpha = 0.5\n");
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    TEST_ASSERT_EQUAL_FLOAT(0.5f, res.theme.top.background.alpha);
    TEST_ASSERT_EQUAL(128, res.theme.top.background.color.a);
    TEST_ASSERT_EQUAL(128, res.theme.body.background.color.a);
    TEST_ASSERT_EQUAL(128, res.theme.bottom.background.color.a);
}

void test_section_alpha_propagates_to_body(void) {
    char *path = create_temp_toml("[theme.bottom]\n"
                                  "background.alpha = 0.5\n");
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    // Should NOT propagate to body unless explicitly implemented
    TEST_ASSERT_EQUAL(180, res.theme.body.background.color.a);
}

typedef struct {
    const char *name;
    const char *toml_content;
    struct {
        uint8_t top_bg_r;
        uint8_t body_font_r;
        uint8_t highlight_bg_r;
        float body_font_size;
        bool has_custom_font_file;
        float global_bg_alpha; // Add this line
        uint8_t body_bg_a;     // Add this line
    } expected;
} ThemeTestCase;

void test_theme_load_various_configs(void) {
    ThemeTestCase cases[] = {
        {.name = "empty file → all defaults",
         .toml_content = "",
         .expected = {20, 130, 100, 16.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "only global background color",
         .toml_content = "theme.background.color = \"#112233\"\n"
                         "theme.font.size = 14.5",
         .expected = {0x11, 130, 100, 14.5f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "top override + highlight override",
         .toml_content = "[theme.top]\n"
                         "background.color = \"#aabbcc\"\n"
                         "[theme.highlight]\n"
                         "background.color = \"#ff0000\"",
         .expected = {0xaa, 130, 0xff, 16.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "full config with font file",
         .toml_content = "theme.font.file = \"/usr/share/fonts/custom.ttf\"\n"
                         "theme.body.font.color = \"#00ff00\"",
         .expected = {20, 0x00, 100, 16.0f, true, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "partial top section",
         .toml_content = "[theme.top]\nbackground.color = \"#112233\"",
         .expected = {0x11, 130, 100, 16.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "invalid color format",
         .toml_content = "theme.background.color = \"#GGHHII\"",
         .expected = {20, 130, 100, 16.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "alpha only",
         .toml_content = "theme.background.alpha = 0.5",
         .expected = {20, 130, 100, 16.0f, false, 0.5f, 128}},
        {.name = "font size only",
         .toml_content = "theme.font.size = 20",
         .expected = {20, 130, 100, 20.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "mixed case hex color",
         .toml_content = "theme.background.color = \"#aAbBcC\"",
         .expected = {0xaa, 130, 100, 16.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "empty font file",
         .toml_content = "theme.font.file = \"\"",
         .expected = {20, 130, 100, 16.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "body font override",
         .toml_content =
             "[theme.body]\nfont.color = \"#ff0000\"\nfont.size = 18",
         .expected = {20, 0xff, 100, 18.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "highlight inherits from body",
         .toml_content = "[theme.body]\nfont.color = \"#123456\"",
         .expected = {20, 0x12, 100, 16.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
        {.name = "global alpha and color",
         .toml_content = "theme.background.color = \"#112233\"\n"
                         "theme.background.alpha = 0.75",
         .expected = {0x11, 130, 100, 16.0f, false, 0.75f,
                      191}}, // 0.75*255 ≈ 191
        {.name = "all sections partial",
         .toml_content = "[theme.top]\n"
                         "background.color = \"#111111\"\n"
                         "[theme.body]\n"
                         "font.size = 14\n"
                         "[theme.bottom]\n"
                         "background.alpha = 0.5\n"
                         "[theme.highlight]\n"
                         "background.color = \"#ffffff\"",
         .expected = {0x11, 130, 0xff, 14.0f, false, 0.0f,
                      THEME_DEFAULT_BG_ALPHA}},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Failed case: %s", cases[i].name);

        char *path = create_temp_toml(cases[i].toml_content);
        TEST_ASSERT_NOT_NULL_MESSAGE(path, msg);

        theme_result_t res = theme_load(path);
        delete_temp_file(path);

        TEST_ASSERT_EQUAL_MESSAGE(THEME_SUCCESS, res.error, msg);

        // Assert alpha values
        TEST_ASSERT_EQUAL_FLOAT_MESSAGE(cases[i].expected.global_bg_alpha,
                                        res.theme.top.background.alpha, msg);
        TEST_ASSERT_EQUAL_MESSAGE(cases[i].expected.body_bg_a,
                                  res.theme.body.background.color.a, msg);

        // Assert other expected values
        TEST_ASSERT_EQUAL_MESSAGE(cases[i].expected.top_bg_r,
                                  res.theme.top.background.color.r, msg);
        TEST_ASSERT_EQUAL_MESSAGE(cases[i].expected.body_font_r,
                                  res.theme.body.font.color.r, msg);
        TEST_ASSERT_EQUAL_MESSAGE(cases[i].expected.highlight_bg_r,
                                  res.theme.highlight.background.color.r, msg);
        TEST_ASSERT_EQUAL_FLOAT_MESSAGE(cases[i].expected.body_font_size,
                                        res.theme.body.font.size, msg);

        if (cases[i].expected.has_custom_font_file) {
            TEST_ASSERT_NOT_NULL_MESSAGE(res.theme.body.font.file, msg);
        } else {
            TEST_ASSERT_TRUE(
                ends_with(res.theme.top.font.file, DEFAULT_FONT_SUFFIX));
        }
    }
}

void test_theme_parse_hex_color_various_formats(void) {
    theme_color_t c = {0};

    parse_hex_color("#112233", &c);
    TEST_ASSERT_EQUAL(17, c.r);
    TEST_ASSERT_EQUAL(34, c.g);
    TEST_ASSERT_EQUAL(51, c.b);
    TEST_ASSERT_EQUAL(0, c.a);
    TEST_ASSERT_FALSE(c.has_alpha);

    theme_color_t c2 = {0};
    parse_hex_color("#aabbccdd", &c2);
    TEST_ASSERT_EQUAL(170, c2.r);
    TEST_ASSERT_EQUAL(187, c2.g);
    TEST_ASSERT_EQUAL(204, c2.b);
    TEST_ASSERT_EQUAL(221, c2.a);
    TEST_ASSERT_TRUE(c2.has_alpha);

    theme_color_t c3 = {0};
    parse_hex_color("invalid", &c3);
    TEST_ASSERT_EQUAL(0, c3.r);
    TEST_ASSERT_EQUAL(0, c3.g);
    TEST_ASSERT_EQUAL(0, c3.b);
    TEST_ASSERT_EQUAL(0, c3.a);
    TEST_ASSERT_FALSE(c3.has_alpha);

    theme_color_t c4 = {0};
    parse_hex_color("#123", &c4);
    TEST_ASSERT_EQUAL(0, c4.r);
    TEST_ASSERT_EQUAL(0, c4.g);
    TEST_ASSERT_EQUAL(0, c4.b);
    TEST_ASSERT_EQUAL(0, c4.a);
    TEST_ASSERT_FALSE(c4.has_alpha);
}

void test_theme_load_from_config_no_home_returns_config_not_found(void) {
    unsetenv("HOME");
    theme_t theme;
    TEST_ASSERT_EQUAL(THEME_CONFIG_NOT_FOUND, theme_load_from_config(&theme));
}

void test_alpha_applies_to_color_when_missing_alpha(void) {
    char *path = create_temp_toml("theme.background.color = \"#112233\"\n"
                                  "theme.background.alpha = 0.5\n");
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    TEST_ASSERT_EQUAL(THEME_SUCCESS, res.error);
    TEST_ASSERT_EQUAL(128, res.theme.body.background.color.a);
    TEST_ASSERT_TRUE(res.theme.body.background.color.has_alpha);
}

void test_font_file_is_duplicated_between_layers(void) {
    char *path = create_temp_toml("theme.font.file = \"/tmp/font.ttf\"\n");
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    TEST_ASSERT_EQUAL(THEME_SUCCESS, res.error);
    TEST_ASSERT_NOT_NULL(res.theme.body.font.file);
    TEST_ASSERT_NOT_NULL(res.theme.top.font.file);
    TEST_ASSERT_NOT_EQUAL(res.theme.body.font.file, res.theme.top.font.file);
}

void test_invalid_type_returns_parsing_error(void) {
    char *path = create_temp_toml("theme.font.size = \"big\"\n");
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    TEST_ASSERT_EQUAL(THEME_PARSING_ERROR, res.error);
}

void test_invalid_toml_syntax_returns_parsing_error(void) {
    char *path = create_temp_toml(
        "theme.background.color = #112233\n"); // missing quotes
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    TEST_ASSERT_EQUAL(THEME_PARSING_ERROR, res.error);
}

void test_memory_allocation_failure(void) {
    // Mock malloc to always fail
    // (Implementation depends on your testing framework/mocking library)
    // TEST_ASSERT_EQUAL(THEME_ALLOC_FAILED, theme_load(...));
}

void test_font_file_not_found(void) {
    char *path =
        create_temp_toml("theme.font.file = \"/nonexistent/font.ttf\"\n");
    theme_result_t res = theme_load(path);
    delete_temp_file(path);

    TEST_ASSERT_EQUAL(THEME_SUCCESS, res.error);
    TEST_ASSERT_NOT_NULL(res.theme.body.font.file);
}

void test_get_config_filepath_with_home(void) {
    setenv("HOME", "/home/user", 1);
    char *path = theme_get_config_filepath();
    TEST_ASSERT_NOT_NULL(path);
    TEST_ASSERT_EQUAL_STRING("/home/user/.config/swindings/config.toml", path);
    free(path);
}

void test_get_config_filepath_without_home(void) {
    unsetenv("HOME");
    char *path = theme_get_config_filepath();
    TEST_ASSERT_NULL(path);
}

void test_theme_error_str(void) {
    TEST_ASSERT_EQUAL_STRING("success", theme_error_str(THEME_SUCCESS));
    TEST_ASSERT_EQUAL_STRING("could not read/write config file",
                             theme_error_str(THEME_IO_ERROR));
    TEST_ASSERT_EQUAL_STRING("memory allocation failed",
                             theme_error_str(THEME_ALLOC_FAILED));
    TEST_ASSERT_EQUAL_STRING("failed to parse config TOML",
                             theme_error_str(THEME_PARSING_ERROR));
    TEST_ASSERT_EQUAL_STRING(
        "return file path could not be determined (HOME not set?)",
        theme_error_str(THEME_CONFIG_NOT_FOUND));
}
