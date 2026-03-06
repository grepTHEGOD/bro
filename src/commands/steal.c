#include <stdio.h>
#include <stdlib.h>

int cmd_steal(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro steal <remote>\n");
        return 1;
    }
    printf("stealing from %s\n", argv[1]);
    return 0;
}
