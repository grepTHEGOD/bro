#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);

int cmd_drip(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "drip: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    int delete_mode = 0;
    int list_mode = 0;
    char *tag_name = NULL;
    char *tag_msg = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            delete_mode = 1;
            if (i + 1 < argc) tag_name = argv[++i];
        } else if (strcmp(argv[i], "-l") == 0) {
            list_mode = 1;
        } else if (argv[i][0] != '-') {
            tag_name = argv[i];
        }
    }
    
    if (list_mode || (!tag_name && !delete_mode)) {
        printf("Listing tags:\n");
        
        char refs_dir[256] = ".bro/refs/tags";
        DIR *dir = opendir(refs_dir);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.') continue;
                printf("%s\n", entry->d_name);
            }
            closedir(dir);
        }
        return 0;
    }
    
    if (delete_mode && tag_name) {
        char tag_path[256];
        snprintf(tag_path, sizeof(tag_path), ".bro/refs/tags/%s", tag_name);
        remove(tag_path);
        printf("Deleted tag '%s'\n", tag_name);
        return 0;
    }
    
    bro_oid oid = {0};
    const char *head_ref = bro_head_read();
    if (head_ref && strncmp(head_ref, "refs/heads/", 11) == 0) {
        bro_refs_read(head_ref, &oid);
    }
    
    char tag_path[256];
    snprintf(tag_path, sizeof(tag_path), ".bro/refs/tags/%s", tag_name);
    
    FILE *f = fopen(tag_path, "w");
    if (f) {
        fprintf(f, "%s\n", bro_oid_to_string(&oid));
        fclose(f);
    }
    
    printf("Created drip '%s' at %s\n", tag_name, bro_oid_to_string(&oid));
    
    return 0;
}
