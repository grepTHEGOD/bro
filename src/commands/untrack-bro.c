#include <stdio.h>

int cmd_untrack_bro(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro untrack-bro <file>\n");
        return 1;
    }
    printf("untracked %s\n", argv[1]);
    return 0;
}
