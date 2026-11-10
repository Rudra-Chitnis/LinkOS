#ifndef LINKOS_LOG_H
#define LINKOS_LOG_H

typedef enum {
    LINKOS_LOG_DEBUG,
    LINKOS_LOG_INFO,
    LINKOS_LOG_WARN,
    LINKOS_LOG_ERROR
} linkos_log_level_t;

void linkos_log(linkos_log_level_t level, const char *message);

#endif
