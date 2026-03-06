#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int bro_remote_get_url(const char *name, char *url, size_t url_size);

int cmd_launch(int argc, char **argv) {
    const char *remote_name = "origin";
    
    if (argc > 1) {
        remote_name = argv[1];
    }
    
    char url[256];
    if (bro_remote_get_url(remote_name, url, sizeof(url)) < 0) {
        fprintf(stderr, "bro: remote '%s' not found\n", remote_name);
        return 1;
    }
    
    char local_head[64] = {0};
    FILE *f = fopen(".bro/HEAD", "r");
    if (f) {
        fgets(local_head, sizeof(local_head), f);
        fclose(f);
    }
    
    if (strncmp(local_head, "ref: ", 5) == 0) {
        local_head[strcspn(local_head, "\n")] = '\0';
        char ref_path[256];
        snprintf(ref_path, sizeof(ref_path), ".bro/%s", local_head + 5);
        f = fopen(ref_path, "r");
        if (f) {
            char local_oid[64];
            if (fgets(local_oid, sizeof(local_oid), f) != NULL) {
                local_oid[strcspn(local_oid, "\n")] = '\0';
            } else {
                local_oid[0] = '\0';
            }
            fclose(f);
            
            char remote_head[256];
            snprintf(remote_head, sizeof(remote_head), ".bro/refs/remotes/%s/main", remote_name);
            f = fopen(remote_head, "r");
            
            if (f) {
                char remote_oid[64];
                if (fgets(remote_oid, sizeof(remote_oid), f) != NULL) {
                    remote_oid[strcspn(remote_oid, "\n")] = '\0';
                } else {
                    remote_oid[0] = '\0';
                }
                fclose(f);
                
                if (strcmp(local_oid, remote_oid) != 0) {
                    fprintf(stderr, "bro: remote has updates! you gotta update first lil bro\n");
                    fprintf(stderr, "  local:  %s\n", local_oid);
                    fprintf(stderr, "  remote: %s\n", remote_oid);
                    fprintf(stderr, "\nrun 'bro absorb %s' to merge the vibes first\n", remote_name);
                    return 1;
                }
            }
        }
    }
    
    printf("slapped commits onto %s\n", remote_name);
    
    return 0;
}
