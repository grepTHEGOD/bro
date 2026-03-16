#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern const char *bro_head_read(void);

int cmd_squad(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "squad: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *head_ref = bro_head_read();
    char current_branch[256] = {0};
    if (head_ref && strncmp(head_ref, "refs/heads/", 11) == 0) {
        strcpy(current_branch, head_ref + 11);
    }
    
    int verbose = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "-vv") == 0) {
            verbose = (strcmp(argv[i], "-vv") == 0) ? 2 : 1;
        }
    }
    
    char refs_dir[256] = ".bro/refs/heads";
    DIR *dir = opendir(refs_dir);
    if (!dir) {
        printf("no squads (branches) yet. create one with 'bro new-squad <name>'\n");
        return 0;
    }
    
    struct dirent *entry;
    char **branches = NULL;
    size_t count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        branches = realloc(branches, (count + 1) * sizeof(char *));
        branches[count++] = strdup(entry->d_name);
    }
    closedir(dir);
    
    if (count == 0) {
        printf("no squads (branches) yet. create one with 'bro new-squad <name>'\n");
        return 0;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (verbose) {
            char ref_path[256];
            snprintf(ref_path, sizeof(ref_path), ".bro/refs/heads/%s", branches[i]);
            FILE *f = fopen(ref_path, "r");
            if (f) {
                char oid[64];
                if (fgets(oid, sizeof(oid), f)) {
                    oid[strcspn(oid, "\n")] = 0;
                    if (strcmp(branches[i], current_branch) == 0) {
                        printf("* %-20s %s\n", branches[i], oid);
                    } else {
                        printf("  %-20s %s\n", branches[i], oid);
                    }
                }
                fclose(f);
            }
        } else {
            if (strcmp(branches[i], current_branch) == 0) {
                printf("* %s\n", branches[i]);
            } else {
                printf("  %s\n", branches[i]);
            }
        }
        free(branches[i]);
    }
    free(branches);
    
    return 0;
}
