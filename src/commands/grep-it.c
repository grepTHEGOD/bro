#include <stdio.h>

int cmd_grep_it(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro grep-it <pattern>\n");
        return 1;
    }
    printf("grepping for: %s\n", argv[1]);
    return 0;
}
