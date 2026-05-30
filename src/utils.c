#include "utils.h"
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

bool file_exists(const char *filepath) {
    return (bool)(access(filepath, F_OK) == 0);
}

char *get_app_prefix(void) {
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
