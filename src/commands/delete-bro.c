#include <stdio.h>

int cmd_delete_bro(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro delete-bro <branch>\n");
        return 1;
    }
    printf("deleted branch %s\n", argv[1]);
    return 0;
}
