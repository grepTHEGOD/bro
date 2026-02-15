// src/main.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cmd_init(int argc, char **argv);
int cmd_hash_slap(int argc, char **argv);
int cmd_peek(int argc, char **argv);
int cmd_vibe_check(int argc, char **argv);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        puts("bro: missing command lil bro\nKnown moves: init, hash-slap, peek, vibe-check");
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {
        return cmd_init(argc - 1, argv + 1);
    }
    if (strcmp(cmd, "hash-slap") == 0) {
        return cmd_hash_slap(argc - 1, argv + 1);
    }
    if (strcmp(cmd, "peek") == 0) {
        return cmd_peek(argc - 1, argv + 1);
    }
    if (strcmp(cmd, "vibe-check") == 0) {
        return cmd_vibe_check(argc - 1, argv + 1);
    }

    fprintf(stderr, "bro: '%s' not recognized... yet 😈\n", cmd);
    return 1;
}


