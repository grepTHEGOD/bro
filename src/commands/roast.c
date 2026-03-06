#include <stdio.h>

int cmd_roast(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro roast <commit>\n");
        return 1;
    }
    printf("roasting %s\n", argv[1]);
    return 0;
}
