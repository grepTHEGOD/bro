#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);

int cmd_undo_vibes(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "undo-vibes: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        fprintf(stderr, "undo-vibes: not on a branch\n");
        return 1;
    }
    
    bro_oid oid = {0};
    if (bro_refs_read(head_ref, &oid) != 0) {
        fprintf(stderr, "undo-vibes: no commits\n");
        return 1;
    }
    
    printf("HEAD is now at %s\n", bro_oid_to_string(&oid));
    
    return 0;
}
