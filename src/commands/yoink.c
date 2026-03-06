#include <stdio.h>

int cmd_yoink(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro yoink <file>\n");
        return 1;
    }
    printf("yoinked %s\n", argv[1]);
    return 0;
}
