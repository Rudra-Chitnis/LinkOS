#ifndef LINKOS_PLATFORM_H
#define LINKOS_PLATFORM_H

const char *linkos_platform_name(void);

/* Writes the per-user LinkOS config directory into buffer.
   Returns 0 on success and -1 if the path cannot be determined or does not fit. */
int linkos_get_config_dir(char *buffer, unsigned long buffer_size);

/* Installs a Ctrl+C / console shutdown handler for the foreground runtime. */
int linkos_platform_runtime_start(void);
int linkos_platform_shutdown_requested(void);
void linkos_platform_sleep_ms(unsigned long milliseconds);
void linkos_platform_runtime_stop(void);

#endif
