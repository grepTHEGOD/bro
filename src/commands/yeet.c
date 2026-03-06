#include <stdio.h>
#include <stdlib.h>

int cmd_yeet(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro yeet <file>\n");
        return 1;
    }
    printf("yeeted %s into the void\n", argv[1]);
    return 0;
}
