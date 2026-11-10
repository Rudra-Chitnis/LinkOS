#include "linkos/log.h"

#include <stdio.h>

static const char *level_name(linkos_log_level_t level)
{
    switch (level) {
        case LINKOS_LOG_DEBUG: return "DEBUG";
        case LINKOS_LOG_INFO:  return "INFO";
        case LINKOS_LOG_WARN:  return "WARN";
        case LINKOS_LOG_ERROR: return "ERROR";
        default:               return "UNKNOWN";
    }
}

void linkos_log(linkos_log_level_t level, const char *message)
{
    if (message == NULL) {
        return;
    }

    fprintf(stderr, "[%s] %s\n", level_name(level), message);
}
