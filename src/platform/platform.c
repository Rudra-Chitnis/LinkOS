#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "linkos/platform.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static volatile LONG shutdown_requested = 0;
#else
#include <time.h>
static volatile sig_atomic_t shutdown_requested = 0;
#endif

#ifdef _WIN32
static BOOL WINAPI console_handler(DWORD event)
{
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT || event == CTRL_CLOSE_EVENT) {
        (void)InterlockedExchange(&shutdown_requested, 1L);
        return TRUE;
    }
    return FALSE;
}
#else
static void signal_handler(int signal_number)
{
    (void)signal_number;
    shutdown_requested = 1;
}

static struct sigaction previous_sigint;
static struct sigaction previous_sigterm;
#endif

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

int linkos_platform_runtime_start(void)
{
#ifdef _WIN32
    (void)InterlockedExchange(&shutdown_requested, 0L);
#else
    shutdown_requested = 0;
#endif
#ifdef _WIN32
    return SetConsoleCtrlHandler(console_handler, TRUE) ? 0 : -1;
#else
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, &previous_sigint) != 0) {
        return -1;
    }
    if (sigaction(SIGTERM, &action, &previous_sigterm) != 0) {
        (void)sigaction(SIGINT, &previous_sigint, NULL);
        return -1;
    }
    return 0;
#endif
}

int linkos_platform_shutdown_requested(void)
{
#ifdef _WIN32
    return InterlockedCompareExchange(&shutdown_requested, 0L, 0L) != 0L;
#else
    return shutdown_requested != 0;
#endif
}

void linkos_platform_sleep_ms(unsigned long milliseconds)
{
#ifdef _WIN32
    Sleep((DWORD)milliseconds);
#else
    struct timespec remaining;
    remaining.tv_sec = (time_t)(milliseconds / 1000UL);
    remaining.tv_nsec = (long)((milliseconds % 1000UL) * 1000000UL);
    while (nanosleep(&remaining, &remaining) != 0 && errno == EINTR &&
           !shutdown_requested) {
        /* Resume after unrelated signals; stop promptly after Ctrl+C/SIGTERM. */
    }
#endif
}

void linkos_platform_runtime_stop(void)
{
#ifdef _WIN32
    (void)SetConsoleCtrlHandler(console_handler, FALSE);
#else
    (void)sigaction(SIGINT, &previous_sigint, NULL);
    (void)sigaction(SIGTERM, &previous_sigterm, NULL);
#endif
}
