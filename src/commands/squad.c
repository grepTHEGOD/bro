#include <stdio.h>

int cmd_squad(int argc, char **argv) {
    if (argc < 2) {
        printf("list your squad (branches)\n");
        return 0;
    }
    printf("squad: %s\n", argv[1]);
    return 0;
}
