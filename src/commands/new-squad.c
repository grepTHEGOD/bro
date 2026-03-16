#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern int bro_refs_update(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);

int cmd_new_squad(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro new-squad <name>\n");
        return 1;
    }
    
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "new-squad: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *branch_name = argv[1];
    
    char ref_path[256];
    snprintf(ref_path, sizeof(ref_path), "refs/heads/%s", branch_name);
    
    if (bro_file_exists(ref_path)) {
        fprintf(stderr, "new-squad: squad '%s' already exists\n", branch_name);
        return 1;
    }
    
    bro_oid oid = {0};
    const char *head_ref = bro_head_read();
    if (head_ref && strncmp(head_ref, "refs/heads/", 11) == 0) {
        bro_refs_read(head_ref, &oid);
    }
    
    bro_refs_update(ref_path, &oid);
    
    printf("Created squad '%s' at %s\n", branch_name, bro_oid_to_string(&oid));
    
    return 0;
}
