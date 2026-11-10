#ifndef LINKOS_PLATFORM_H
#define LINKOS_PLATFORM_H

const char *linkos_platform_name(void);

/* Writes the per-user LinkOS config directory into buffer.
   Returns 0 on success and -1 if the path cannot be determined or does not fit. */
int linkos_get_config_dir(char *buffer, unsigned long buffer_size);

#endif
