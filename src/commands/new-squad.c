#include <stdio.h>

int cmd_new_squad(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro new-squad <name>\n");
        return 1;
    }
    printf("created new squad: %s\n", argv[1]);
    return 0;
}
