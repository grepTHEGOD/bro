#include <stdio.h>

int cmd_rip_branch(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro rip-branch <branch>\n");
        return 1;
    }
    printf("ripped branch %s\n", argv[1]);
    return 0;
}
