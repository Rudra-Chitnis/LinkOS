#include "linkos/commands.h"
#include "linkos/log.h"
#include "linkos/platform.h"

#include <stdio.h>
#include <string.h>

#ifndef LINKOS_VERSION
#define LINKOS_VERSION "0.1.0-dev"
#endif

static void print_help(void)
{
    puts("LinkOS - cross-platform peer-to-peer file sharing");
    puts("");
    puts("Usage:");
    puts("  linkos --version");
    puts("  linkos help");
    puts("  linkos doctor");
    puts("  linkos start");
    puts("");
    puts("Run 'linkos doctor' to inspect this installation.");
}

static int run_doctor(void)
{
    char config_dir[1024];

    printf("LinkOS %s\n", LINKOS_VERSION);
    printf("Platform: %s\n", linkos_platform_name());

    if (linkos_get_config_dir(config_dir, (unsigned long)sizeof(config_dir)) == 0) {
        printf("Config directory: %s\n", config_dir);
    } else {
        puts("Config directory: unavailable");
        linkos_log(LINKOS_LOG_WARN, "Could not determine the user config directory.");
        return 1;
    }

    puts("Runtime: foreground");
    puts("Networking: not implemented yet");
    return 0;
}

static int run_start(void)
{
    if (linkos_platform_runtime_start() != 0) {
        fputs("Could not install the shutdown handler.\n", stderr);
        return 1;
    }

    puts("LinkOS is running in the foreground. Press Ctrl+C to stop.");
    puts("Networking is not implemented; no listener has been started.");
    (void)fflush(stdout);
    linkos_log(LINKOS_LOG_INFO, "Foreground runtime started.");
    while (!linkos_platform_shutdown_requested()) {
        linkos_platform_sleep_ms(250UL);
    }
    linkos_log(LINKOS_LOG_INFO, "Foreground runtime stopped.");
    puts("LinkOS stopped.");
    linkos_platform_runtime_stop();
    return 0;
}

int linkos_run_command(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "help") == 0 ||
        strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("LinkOS %s\n", LINKOS_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "doctor") == 0) {
        return run_doctor();
    }

    if (strcmp(argv[1], "start") == 0) {
        return run_start();
    }

    fprintf(stderr, "Unknown command: %s\n\n", argv[1]);
    print_help();
    return 2;
}
