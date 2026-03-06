#include <stdio.h>

int cmd_absorb(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro absorb <branch>\n");
        return 1;
    }
    printf("absorbed %s\n", argv[1]);
    return 0;
}
