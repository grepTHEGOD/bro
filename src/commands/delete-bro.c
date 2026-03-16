#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);

int cmd_delete_bro(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro delete-bro <file>...\n");
        return 1;
    }
    
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "delete-bro: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        const char *path = argv[i];
        
        if (!bro_file_exists(path)) {
            fprintf(stderr, "delete-bro: '%s' not found\n", path);
            continue;
        }
        
        if (unlink(path) != 0) {
            fprintf(stderr, "delete-bro: could not delete '%s'\n", path);
            continue;
        }
        
        FILE *f = fopen(".bro/index", "r");
        FILE *tmp = fopen(".bro/index.tmp", "w");
        char line[512];
        int found = 0;
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line + 41, path) == line + 41) {
                found = 1;
                continue;
            }
            fputs(line, tmp);
        }
        fclose(f);
        fclose(tmp);
        rename(".bro/index.tmp", ".bro/index");
        
        printf("deleted: %s\n", path);
    }
    
    return 0;
}
