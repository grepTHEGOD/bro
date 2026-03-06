#include <stdio.h>

int cmd_hide(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro hide <file>\n");
        return 1;
    }
    printf("hiding %s\n", argv[1]);
    return 0;
}
