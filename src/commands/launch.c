#include "../core/types.h"
#include "github.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);
extern int bro_remote_get_url(const char *name, char *url, size_t url_size);

int cmd_launch(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "launch: not a bro repository. Run 'bro init' first.\n");
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
        fprintf(stderr, "launch: remote '%s' not found. Add it with:\n", remote_name);
        fprintf(stderr, "  bro config-vibe remote.%s.url <github-url>\n", remote_name);
        return 1;
    }
    
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        fprintf(stderr, "launch: cannot push from detached HEAD\n");
        return 1;
    }
    
    bro_oid oid;
    if (bro_refs_read(head_ref, &oid) != 0) {
        fprintf(stderr, "launch: nothing to push\n");
        return 1;
    }
    
    char branch_name[64];
    strcpy(branch_name, head_ref + 11);
    
    bro_github_repo gh;
    if (bro_github_parse_url(remote_url, &gh) == 0 && gh.is_github) {
        if (bro_github_push(&gh, branch_name, bro_oid_to_string(&oid)) != 0) {
            fprintf(stderr, "launch: GitHub push failed\n");
            if (gh.token) free(gh.token);
            return 1;
        }
        if (gh.token) free(gh.token);
    } else {
        char refs_dir[256];
        snprintf(refs_dir, sizeof(refs_dir), ".bro/refs/remotes/%s", remote_name);
        mkdir(refs_dir, 0755);
        
        char ref_path[256];
        snprintf(ref_path, sizeof(ref_path), "%s/%s", refs_dir, branch_name);
        
        FILE *rf = fopen(ref_path, "w");
        if (rf) {
            fprintf(rf, "%s\n", bro_oid_to_string(&oid));
            fclose(rf);
        }
        
        printf("To %s\n", remote_url);
        printf(" * [new branch] %s -> %s\n", branch_name, branch_name);
    }
    
    return 0;
}
