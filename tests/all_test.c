#include "unity.h"
#include <stdlib.h>
#include <string.h>

static char *saved_home;
static char *saved_xdg;

void setUp(void) {
    char *h = getenv("HOME");
    saved_home = h ? strdup(h) : NULL;
    char *x = getenv("XDG_CONFIG_HOME");
    saved_xdg = x ? strdup(x) : NULL;
}

void tearDown(void) {
    if (saved_home) {
        setenv("HOME", saved_home, 1);
        free(saved_home);
    } else
        unsetenv("HOME");
    if (saved_xdg) {
        setenv("XDG_CONFIG_HOME", saved_xdg, 1);
        free(saved_xdg);
    } else
        unsetenv("XDG_CONFIG_HOME");
}

extern void test_in_intlist(void);
extern void test_get_segments(void);
extern void test_theme_load_from_config_no_home_returns_config_not_found(void);
extern void test_get_config_filepath_with_home(void);
extern void test_get_config_filepath_without_home(void);
extern void
test_theme_load_nonexistent_file_creates_it_and_loads_defaults(void);
extern void test_theme_load_empty_file_returns_all_defaults(void);
extern void test_theme_load_various_configs(void);
extern void test_theme_parse_hex_color_various_formats(void);
extern void test_alpha_applies_to_color_when_missing_alpha(void);
extern void test_font_file_is_duplicated_between_layers(void);
extern void test_invalid_type_returns_parsing_error(void);
extern void test_invalid_toml_syntax_returns_parsing_error(void);
extern void test_font_file_not_found(void);
extern void test_theme_error_str(void);

extern void test_global_overrides_section_if_not_set(void);
extern void test_section_overrides_global(void);
extern void test_global_alpha_propagates_to_sections(void);
extern void test_section_alpha_propagates_to_body(void);

extern void test_file_exists_for_existing_file(void);
extern void test_file_exists_for_missing_file(void);
extern void test_sway_filepath_home_sway_dir(void);
extern void test_sway_filepath_xdg_config_home_sway(void);
extern void test_sway_filepath_home_sway_beats_xdg(void);
extern void test_sway_filepath_falls_back_to_i3(void);
extern void test_sway_filepath_none_exist_returns_null(void);
extern void test_sway_filepath_no_home_no_xdg_returns_null(void);
extern void test_sway_filepath_xdg_config_home_i3(void);
extern void test_include_ignored_when_follow_includes_false(void);
extern void test_include_resolved_when_follow_includes_true(void);
extern void test_include_glob_resolves_multiple_files(void);
extern void test_include_missing_file_is_silently_skipped(void);
extern void test_remove_flags(void);
extern void test_normalize_space(void);

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_in_intlist);
    RUN_TEST(test_get_segments);

    RUN_TEST(test_theme_load_from_config_no_home_returns_config_not_found);
    RUN_TEST(test_get_config_filepath_with_home);
    RUN_TEST(test_get_config_filepath_without_home);
    RUN_TEST(test_theme_load_nonexistent_file_creates_it_and_loads_defaults);
    RUN_TEST(test_theme_load_empty_file_returns_all_defaults);
    RUN_TEST(test_theme_load_various_configs);
    RUN_TEST(test_theme_parse_hex_color_various_formats);
    RUN_TEST(test_alpha_applies_to_color_when_missing_alpha);
    RUN_TEST(test_font_file_is_duplicated_between_layers);
    RUN_TEST(test_invalid_type_returns_parsing_error);
    RUN_TEST(test_invalid_toml_syntax_returns_parsing_error);
    RUN_TEST(test_font_file_not_found);
    RUN_TEST(test_theme_error_str);

    RUN_TEST(test_global_overrides_section_if_not_set);
    RUN_TEST(test_section_overrides_global);
    RUN_TEST(test_global_alpha_propagates_to_sections);
    RUN_TEST(test_section_alpha_propagates_to_body);

    RUN_TEST(test_file_exists_for_existing_file);
    RUN_TEST(test_file_exists_for_missing_file);
    RUN_TEST(test_sway_filepath_home_sway_dir);
    RUN_TEST(test_sway_filepath_xdg_config_home_sway);
    RUN_TEST(test_sway_filepath_home_sway_beats_xdg);
    RUN_TEST(test_sway_filepath_falls_back_to_i3);
    RUN_TEST(test_sway_filepath_none_exist_returns_null);
    RUN_TEST(test_sway_filepath_no_home_no_xdg_returns_null);
    RUN_TEST(test_sway_filepath_xdg_config_home_i3);
    RUN_TEST(test_remove_flags);
    RUN_TEST(test_include_ignored_when_follow_includes_false);
    RUN_TEST(test_include_resolved_when_follow_includes_true);
    RUN_TEST(test_include_glob_resolves_multiple_files);
    RUN_TEST(test_include_missing_file_is_silently_skipped);
    RUN_TEST(test_normalize_space);

    return UNITY_END();
}
