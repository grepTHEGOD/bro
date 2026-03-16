#include "core/types.h"
#include <stdio.h>
#include <string.h>

extern int bro_file_exists(const char *path);
extern char *bro_read_file(const char *path, size_t *len);

int cmd_diff_bro(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "diff-bro: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    FILE *f = fopen(".bro/index", "r");
    if (!f) {
        printf("no changes staged\n");
        return 0;
    }
    
    printf("diff --git a/<cached> b/<cached>\n");
    printf("--- a/<cached>\n");
    printf("+++ b/<cached>\n");
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) < 41) continue;
        
        char *path = line + 41;
        printf("+ %s\n", path);
    }
    
    fclose(f);
    
    return 0;
}
