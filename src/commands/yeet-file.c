#include <stdio.h>

int cmd_yeet_file(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro yeet-file <file>\n");
        return 1;
    }
    printf("yeeted file %s\n", argv[1]);
    return 0;
}
