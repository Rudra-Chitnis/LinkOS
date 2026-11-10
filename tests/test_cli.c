#include "linkos/commands.h"
#include "linkos/platform.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    } else {
        printf("PASS: %s\n", name);
    }
}

int main(void)
{
    char path[1024];
    check(strcmp(linkos_platform_name(), "Unknown") != 0, "platform is identified");
    check(linkos_get_config_dir(path, (unsigned long)sizeof(path)) == 0,
          "user config path is available");

    {
        char *version_args[] = {"linkos", "--version"};
        check(linkos_run_command(2, version_args) == 0, "--version succeeds");
    }
    {
        char *help_args[] = {"linkos", "help"};
        check(linkos_run_command(2, help_args) == 0, "help succeeds");
    }

    if (failures != 0) {
        fprintf(stderr, "%d test(s) failed.\n", failures);
        return 1;
    }
    puts("All tests passed.");
    return 0;
}
