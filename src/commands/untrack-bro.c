#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int bro_file_exists(const char *path);

int cmd_untrack_bro(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "untrack-bro: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    if (argc < 2) {
        fprintf(stderr, "Usage: bro untrack-bro <file>...\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        const char *path = argv[i];
        
        FILE *f = fopen(".bro/index", "r");
        FILE *tmp = fopen(".bro/index.tmp", "w");
        
        char line[512];
        int found = 0;
        
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line + 41, path, strlen(path)) == 0 && (line[41 + strlen(path)] == '\n' || line[41 + strlen(path)] == 0)) {
                found = 1;
                continue;
            }
            fputs(line, tmp);
        }
        
        fclose(f);
        fclose(tmp);
        
        rename(".bro/index.tmp", ".bro/index");
        
        if (found) {
            printf("Untracked: %s\n", path);
        } else {
            printf("Was not tracking: %s\n", path);
        }
    }
    
    return 0;
}
