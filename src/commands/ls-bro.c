#include <stdio.h>
#include <string.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);

int cmd_ls_bro(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "ls-bro: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    int cached = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cached") == 0) cached = 1;
    }
    
    FILE *f = fopen(".bro/index", "r");
    if (!f) return 0;
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) < 41) continue;
        printf("%s\n", line + 41);
    }
    fclose(f);
    
    return 0;
}
