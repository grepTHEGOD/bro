#include <stdio.h>

int cmd_search_bro(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro search-bro <pattern>\n");
        return 1;
    }
    printf("searching for '%s'\n", argv[1]);
    return 0;
}
