#include <stdio.h>

int cmd_slap(int argc, char **argv) {
    if (argc < 2) {
        printf("slap: give a file some love\n");
        return 0;
    }
    printf("slapped %s\n", argv[1]);
    return 0;
}
