#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int bro_file_exists(const char *path);

int cmd_yeet_file(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "yeet-file: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    if (argc < 3) {
        fprintf(stderr, "Usage: bro yeet-file <source> <dest>\n");
        return 1;
    }
    
    const char *src = argv[1];
    const char *dst = argv[2];
    
    if (!bro_file_exists(src)) {
        fprintf(stderr, "yeet-file: '%s' does not exist\n", src);
        return 1;
    }
    
    if (bro_file_exists(dst)) {
        fprintf(stderr, "yeet-file: '%s' already exists\n", dst);
        return 1;
    }
    
    if (rename(src, dst) != 0) {
        fprintf(stderr, "yeet-file: could not rename '%s' to '%s'\n", src, dst);
        return 1;
    }
    
    FILE *f = fopen(".bro/index", "r");
    FILE *tmp = fopen(".bro/index.tmp", "w");
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line + 41, src, strlen(src)) == 0) {
            char oid_str[41];
            strncpy(oid_str, line, 40);
            fprintf(tmp, "%s %s\n", oid_str, dst);
        } else {
            fputs(line, tmp);
        }
    }
    
    fclose(f);
    fclose(tmp);
    
    rename(".bro/index.tmp", ".bro/index");
    
    printf("Renamed: %s -> %s\n", src, dst);
    
    return 0;
}
