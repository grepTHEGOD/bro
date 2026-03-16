#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern int bro_refs_update(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);
extern int bro_remote_get_url(const char *name, char *url, size_t url_size);

int cmd_absorb(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "absorb: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *remote_name = "origin";
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            remote_name = argv[i];
        }
    }
    
    char remote_url[256];
    if (bro_remote_get_url(remote_name, remote_url, sizeof(remote_url)) < 0) {
        fprintf(stderr, "absorb: remote '%s' not found\n", remote_name);
        return 1;
    }
    
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        fprintf(stderr, "absorb: cannot pull in detached HEAD\n");
        return 1;
    }
    
    char branch_name[64];
    strcpy(branch_name, head_ref + 11);
    
    char remote_ref[256];
    snprintf(remote_ref, sizeof(remote_ref), ".bro/refs/remotes/%s/%s", remote_name, branch_name);
    
    bro_oid remote_oid = {0};
    FILE *rf = fopen(remote_ref, "r");
    if (!rf) {
        fprintf(stderr, "absorb: no remote tracking branch for '%s'\n", branch_name);
        return 1;
    }
    
    char oid_str[64];
    if (fgets(oid_str, sizeof(oid_str), rf)) {
        oid_str[strcspn(oid_str, "\n")] = 0;
        for (int i = 0; i < 20 && i * 2 < (int)strlen(oid_str); i++) {
            unsigned int val;
            sscanf(oid_str + i * 2, "%02x", &val);
            remote_oid.sha1[i] = val;
        }
    }
    fclose(rf);
    
    bro_oid local_oid = {0};
    bro_refs_read(head_ref, &local_oid);
    
    if (bro_oid_cmp(&local_oid, &remote_oid) == 0) {
        printf("Already up to date.\n");
        return 0;
    }
    
    printf("From %s\n", remote_url);
    printf("   %s..%s  %s -> %s\n", 
           bro_oid_to_string(&local_oid), bro_oid_to_string(&remote_oid), 
           branch_name, branch_name);
    
    bro_refs_update(head_ref, &remote_oid);
    
    printf("Absorbed vibes from %s\n", remote_name);
    
    return 0;
}
