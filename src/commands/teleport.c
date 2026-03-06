#include <stdio.h>

int cmd_teleport(int argc, char **argv) {
    if (argc < 2) {
        printf("teleport to a commit\n");
        return 0;
    }
    printf("teleported to %s\n", argv[1]);
    return 0;
}
