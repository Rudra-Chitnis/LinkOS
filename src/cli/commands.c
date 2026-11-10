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
        puts("LinkOS foreground runtime is not implemented in this milestone.");
        puts("No network listener has been started.");
        return 2;
    }

    fprintf(stderr, "Unknown command: %s\n\n", argv[1]);
    print_help();
    return 2;
}
