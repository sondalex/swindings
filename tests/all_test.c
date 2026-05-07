#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

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

    return UNITY_END();
}
