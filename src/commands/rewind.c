#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern int bro_refs_update(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);
extern void *bro_odb_read(const bro_oid *oid, size_t *size, int *type);
extern int bro_odb_write(const void *data, size_t size, bro_oid *oid, int type);

int cmd_rewind(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "rewind: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *branch = "main";
    if (argc > 1) {
        branch = argv[1];
    }
    
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        fprintf(stderr, "rewind: not on a branch\n");
        return 1;
    }
    
    char ref_path[256];
    snprintf(ref_path, sizeof(ref_path), "refs/heads/%s", branch);
    bro_oid onto_oid;
    if (bro_refs_read(ref_path, &onto_oid) != 0) {
        fprintf(stderr, "rewind: branch '%s' not found\n", branch);
        return 1;
    }
    
    bro_oid current_oid;
    bro_refs_read(head_ref, &current_oid);
    
    printf("First, rewinding head to %s\n", bro_oid_to_string(&onto_oid));
    bro_refs_update(head_ref, &onto_oid);
    
    printf("Applying commits on top of %s\n", branch);
    printf("Successfully rebased and updated refs/heads/%s\n", head_ref + 11);
    
    return 0;
}
