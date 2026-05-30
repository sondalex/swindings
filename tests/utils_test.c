#include "unity.h"
#include "utils.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

void test_file_exists_for_existing_file(void) {
    char tmpl[] = "/tmp/config_test_exists_XXXXXX";
    int fd = mkstemp(tmpl);
    TEST_ASSERT_NOT_EQUAL(-1, fd);
    close(fd);
    TEST_ASSERT_TRUE(file_exists(tmpl));
    if (remove(tmpl))
        perror(tmpl);
}

void test_file_exists_for_missing_file(void) {
    TEST_ASSERT_FALSE(
        file_exists("/tmp/config_test_definitely_does_not_exist_xyz123"));
}
