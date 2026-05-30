#define _XOPEN_SOURCE 700
#include "config.h"
#include "ftw.h"
#include "structures.h"
#include "unity.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHECKED_SNPRINTF(buf, fmt, ...)                                        \
    do {                                                                       \
        if (snprintf(buf, sizeof(buf), fmt, __VA_ARGS__) < 0)                  \
            perror("snprintf failed");                                         \
    } while (0)

static char *make_tmpdir(void) {
    char tmpl[] = "/tmp/config_test_XXXXXX";
    char *p = mkdtemp(tmpl);
    if (!p)
        return NULL;

    return strdup(p);
}

static void mkdir_p(const char *dir) {
    char buf[PATH_MAX];
    CHECKED_SNPRINTF(buf, "%s", dir);
    for (char *p = buf + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(buf, 0755);
            *p = '/';
        }
    }
    mkdir(buf, 0755);
}

static void touch(const char *path) {
    FILE *f = fopen(path, "w");
    if (f)
        if (fclose(f) == -1)
            return;
}

int remove_dir(const char *path, const struct stat *sb, int typeflag,
               struct FTW *ftwbuf) {
    return remove(path);
}

int remove_recursively(const char *path) {
    return nftw(path, remove_dir, 64, FTW_DEPTH | FTW_PHYS);
}

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    if (fputs(content, f) != 0)
        perror("write failed");
    if (fclose(f) != 0)
        perror("write failed");
}

void test_sway_filepath_home_sway_dir(void) {
    char *base = make_tmpdir();

    char dir[PATH_MAX], path[PATH_MAX];
    CHECKED_SNPRINTF(dir, "%s/.sway", base);
    CHECKED_SNPRINTF(path, "%s/config", dir);
    mkdir_p(dir);
    touch(path);

    unsetenv("XDG_CONFIG_HOME");
    setenv("HOME", base, 1);

    char *result = config_get_sway_filepath();
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(path, result);
    free(result);

    remove_recursively(base);
    free(base);
}

void test_sway_filepath_xdg_config_home_sway(void) {
    char *base = make_tmpdir();

    char dir[PATH_MAX], path[PATH_MAX];
    CHECKED_SNPRINTF(dir, "%s/sway", base);
    CHECKED_SNPRINTF(path, "%s/config", dir);
    mkdir_p(dir);
    touch(path);

    char *home = make_tmpdir();
    setenv("HOME", home, 1);
    setenv("XDG_CONFIG_HOME", base, 1);

    char *result = config_get_sway_filepath();
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(path, result);
    free(result);

    remove_recursively(base);
    remove_recursively(home);
    free(base);
    free(home);
}

void test_sway_filepath_home_sway_beats_xdg(void) {
    char *home = make_tmpdir();
    char *xdg = make_tmpdir();

    char home_dir[PATH_MAX], home_path[PATH_MAX];
    CHECKED_SNPRINTF(home_dir, "%s/.sway", home);
    CHECKED_SNPRINTF(home_path, "%s/config", home_dir);
    mkdir_p(home_dir);
    touch(home_path);

    char xdg_dir[PATH_MAX], xdg_path[PATH_MAX];
    CHECKED_SNPRINTF(xdg_dir, "%s/sway", xdg);
    CHECKED_SNPRINTF(xdg_path, "%s/config", xdg_dir);
    mkdir_p(xdg_dir);
    touch(xdg_path);

    setenv("HOME", home, 1);
    setenv("XDG_CONFIG_HOME", xdg, 1);

    char *result = config_get_sway_filepath();
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(home_path, result);
    free(result);

    remove_recursively(home);
    remove_recursively(xdg);
    free(home);
    free(xdg);
}

void test_sway_filepath_falls_back_to_i3(void) {
    char *home = make_tmpdir();

    char dir[PATH_MAX], path[PATH_MAX];
    CHECKED_SNPRINTF(dir, "%s/.i3", home);
    CHECKED_SNPRINTF(path, "%s/config", dir);
    mkdir_p(dir);
    touch(path);

    setenv("HOME", home, 1);
    unsetenv("XDG_CONFIG_HOME");

    char *result = config_get_sway_filepath();
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(path, result);
    free(result);

    remove_recursively(home);
    free(home);
}

void test_sway_filepath_none_exist_returns_null(void) {
    char *home = make_tmpdir();

    setenv("HOME", home, 1);
    unsetenv("XDG_CONFIG_HOME");

    char *result = config_get_sway_filepath();
    TEST_ASSERT_NULL(result);

    remove_recursively(home);
    free(home);
}

void test_sway_filepath_no_home_no_xdg_returns_null(void) {
    unsetenv("HOME");
    unsetenv("XDG_CONFIG_HOME");

    char *result = config_get_sway_filepath();
    TEST_ASSERT_NULL(result);
}

void test_sway_filepath_xdg_config_home_i3(void) {
    char *home = make_tmpdir();
    char *xdg = make_tmpdir();

    char dir[PATH_MAX], path[PATH_MAX];
    CHECKED_SNPRINTF(dir, "%s/i3", xdg);
    CHECKED_SNPRINTF(path, "%s/config", dir);
    mkdir_p(dir);
    touch(path);

    setenv("HOME", home, 1);
    setenv("XDG_CONFIG_HOME", xdg, 1);

    char *result = config_get_sway_filepath();
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(path, result);
    free(result);

    remove_recursively(home);
    remove_recursively(xdg);
    free(home);
    free(xdg);
}

void test_include_ignored_when_follow_includes_false(void) {
    char *base = make_tmpdir();

    char inc_path[PATH_MAX];
    CHECKED_SNPRINTF(inc_path, "%s/extra.conf", base);
    write_file(inc_path, "bindsym $mod+x exec xterm # Extra terminal\n");

    char main_path[PATH_MAX];
    CHECKED_SNPRINTF(main_path, "%s/config", base);
    char main_content[PATH_MAX + 64];
    CHECKED_SNPRINTF(main_content,
                     "bindsym $mod+Return exec foot # Terminal\ninclude %s\n",
                     inc_path);
    write_file(main_path, main_content);

    stringlist_t list;
    stringlist_init(&list);
    config_error_t err = config_read_file(main_path, &list, false);
    TEST_ASSERT_EQUAL_INT(CONFIG_SUCCESS, err);
    TEST_ASSERT_EQUAL_size_t(1, list.count);
    TEST_ASSERT_TRUE(strncmp(list.items[0], "bindsym $mod+Return", 19) == 0);

    stringlist_free(&list);
    remove_recursively(base);
    free(base);
}

void test_include_resolved_when_follow_includes_true(void) {
    char *base = make_tmpdir();

    char inc_path[PATH_MAX];
    CHECKED_SNPRINTF(inc_path, "%s/extra.conf", base);
    write_file(inc_path, "bindsym $mod+x exec xterm # Extra terminal\n");

    char main_path[PATH_MAX];
    CHECKED_SNPRINTF(main_path, "%s/config", base);
    char main_content[PATH_MAX + 64];
    CHECKED_SNPRINTF(main_content,
                     "bindsym $mod+Return exec foot # Terminal\ninclude %s\n",
                     inc_path);
    write_file(main_path, main_content);

    stringlist_t list;
    stringlist_init(&list);
    config_error_t err = config_read_file(main_path, &list, true);
    TEST_ASSERT_EQUAL_INT(CONFIG_SUCCESS, err);
    TEST_ASSERT_EQUAL_size_t(2, list.count);

    stringlist_free(&list);
    remove_recursively(base);
    free(base);
}

void test_include_glob_resolves_multiple_files(void) {
    char *base = make_tmpdir();

    char conf_dir[PATH_MAX];
    CHECKED_SNPRINTF(conf_dir, "%s/conf.d", base);
    mkdir_p(conf_dir);

    char f1[PATH_MAX], f2[PATH_MAX];
    CHECKED_SNPRINTF(f1, "%s/01.conf", conf_dir);
    CHECKED_SNPRINTF(f2, "%s/02.conf", conf_dir);
    write_file(f1, "bindsym $mod+1 workspace 1 # WS 1\n");
    write_file(f2, "bindsym $mod+2 workspace 2 # WS 2\n");

    char main_path[PATH_MAX];
    CHECKED_SNPRINTF(main_path, "%s/config", base);
    char main_content[PATH_MAX + 64];
    CHECKED_SNPRINTF(main_content, "include %s/*.conf\n", conf_dir);
    write_file(main_path, main_content);

    stringlist_t list;
    stringlist_init(&list);
    config_error_t err = config_read_file(main_path, &list, true);
    TEST_ASSERT_EQUAL_INT(CONFIG_SUCCESS, err);
    TEST_ASSERT_EQUAL_size_t(2, list.count);

    stringlist_free(&list);
    remove_recursively(base);
    free(base);
}

void test_include_missing_file_is_silently_skipped(void) {
    char *base = make_tmpdir();

    char main_path[PATH_MAX];
    CHECKED_SNPRINTF(main_path, "%s/config", base);
    write_file(main_path,
               "bindsym $mod+Return exec foot # Terminal\n"
               "include /nonexistent/path/that/does/not/exist.conf\n");

    stringlist_t list;
    stringlist_init(&list);
    config_error_t err = config_read_file(main_path, &list, true);
    TEST_ASSERT_EQUAL_INT(CONFIG_SUCCESS, err);
    TEST_ASSERT_EQUAL_size_t(1, list.count);

    stringlist_free(&list);
    remove_recursively(base);
    free(base);
}
