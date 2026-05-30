#define _XOPEN_SOURCE 700
#include "config.h"
#include "ftw.h"
#include "unity.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *make_tmpdir(void) {
    char tmpl[] = "/tmp/config_test_XXXXXX";
    char *p = mkdtemp(tmpl);
    if (!p)
        return NULL;

    return strdup(p);
}

static void mkdir_p(const char *dir) {
    char buf[PATH_MAX];
    if (snprintf(buf, sizeof(buf), "%s", dir) < 0)
        return;
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

void test_sway_filepath_home_sway_dir(void) {
    char *base = make_tmpdir();

    char dir[PATH_MAX], path[PATH_MAX];
    if (snprintf(dir, sizeof(dir), "%s/.sway", base) < 0)
        perror("snprintf failed");
    if (snprintf(path, sizeof(path), "%s/config", dir) < 0)
        perror("snprintf failed");
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
    if (snprintf(dir, sizeof(dir), "%s/sway", base) < 0)
        perror("snprintf failed");
    if (snprintf(path, sizeof(path), "%s/config", dir) < 0)
        perror("snprintf failed");
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
    if (snprintf(home_dir, sizeof(home_dir), "%s/.sway", home) < 0)
        perror("snprintf failed");
    if (snprintf(home_path, sizeof(home_path), "%s/config", home_dir) < 0)
        perror("snprintf failed");
    mkdir_p(home_dir);
    touch(home_path);

    char xdg_dir[PATH_MAX], xdg_path[PATH_MAX];
    if (snprintf(xdg_dir, sizeof(xdg_dir), "%s/sway", xdg) < 0)
        perror("snprintf failed");
    if (snprintf(xdg_path, sizeof(xdg_path), "%s/config", xdg_dir) < 0)
        perror("snprintf failed");
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
    if (snprintf(dir, sizeof(dir), "%s/.i3", home) < 0)
        perror("snprinf failed");
    if (snprintf(path, sizeof(path), "%s/config", dir) < 0)
        perror("snprintf failed");
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
    if (snprintf(dir, sizeof(dir), "%s/i3", xdg) < 0)
        perror("snprintf failed");
    if (snprintf(path, sizeof(path), "%s/config", dir) < 0)
        perror("snprintf failed");
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
