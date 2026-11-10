#include "linkos/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define LINKOS_PATH_SEPARATOR '\\'
#else
#define LINKOS_PATH_SEPARATOR '/'
#endif

const char *linkos_platform_name(void)
{
#ifdef _WIN32
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown";
#endif
}

static int copy_path(char *buffer, unsigned long buffer_size, const char *base,
                     const char *suffix)
{
    size_t base_len;
    size_t suffix_len;

    if (buffer == NULL || base == NULL || suffix == NULL || buffer_size == 0UL) {
        return -1;
    }

    base_len = strlen(base);
    suffix_len = strlen(suffix);

    if (base_len + suffix_len + 2U > (size_t)buffer_size) {
        return -1;
    }

    memcpy(buffer, base, base_len);
    if (base_len > 0U && base[base_len - 1U] != '/' && base[base_len - 1U] != '\\') {
        buffer[base_len++] = LINKOS_PATH_SEPARATOR;
    }
    memcpy(buffer + base_len, suffix, suffix_len + 1U);
    return 0;
}

int linkos_get_config_dir(char *buffer, unsigned long buffer_size)
{
#ifdef _WIN32
    const char *base = getenv("APPDATA");
    if (base == NULL || base[0] == '\0') {
        return -1;
    }
    return copy_path(buffer, buffer_size, base, "LinkOS");
#elif defined(__APPLE__)
    const char *home = getenv("HOME");
    if (home == NULL || home[0] == '\0') {
        return -1;
    }
    return copy_path(buffer, buffer_size, home, "Library/Application Support/LinkOS");
#else
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");

    if (xdg != NULL && xdg[0] != '\0') {
        return copy_path(buffer, buffer_size, xdg, "linkos");
    }
    if (home == NULL || home[0] == '\0') {
        return -1;
    }
    return copy_path(buffer, buffer_size, home, ".config/linkos");
#endif
}
