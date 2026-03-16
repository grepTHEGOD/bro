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

int cmd_fuse(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "fuse: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *branch = "main";
    if (argc > 1) {
        branch = argv[1];
    }
    
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        fprintf(stderr, "fuse: not on a branch\n");
        return 1;
    }
    
    bro_oid our_oid;
    bro_refs_read(head_ref, &our_oid);
    
    char ref_path[256];
    snprintf(ref_path, sizeof(ref_path), "refs/heads/%s", branch);
    bro_oid their_oid;
    if (bro_refs_read(ref_path, &their_oid) != 0) {
        fprintf(stderr, "fuse: branch '%s' not found\n", branch);
        return 1;
    }
    
    if (bro_oid_cmp(&our_oid, &their_oid) == 0) {
        printf("Already up to date.\n");
        return 0;
    }
    
    size_t their_size;
    int their_type;
    void *their_data = bro_odb_read(&their_oid, &their_size, &their_type);
    if (!their_data || their_type != 3) {
        fprintf(stderr, "fuse: could not read commit\n");
        free(their_data);
        return 1;
    }
    
    char *their_content = (char *)their_data;
    their_content[their_size] = 0;
    
    char *tree_line = strstr(their_content, "tree ");
    if (!tree_line) {
        fprintf(stderr, "fuse: commit has no tree\n");
        free(their_data);
        return 1;
    }
    
    char tree_oid_str[41] = {0};
    char *end = strchr(tree_line + 5, '\n');
    if (end) strncpy(tree_oid_str, tree_line + 5, end - (tree_line + 5));
    else strncpy(tree_oid_str, tree_line + 5, 40);
    
    const char *author = getenv("BRO_AUTHOR");
    if (!author) author = "bro <bro@localhost>";
    
    uint64_t timestamp = time(NULL);
    
    char *commit_content = malloc(1024);
    int offset = 0;
    
    offset += snprintf(commit_content + offset, 1024 - offset, "tree %s\n", tree_oid_str);
    offset += snprintf(commit_content + offset, 1024 - offset, "parent %s\n", bro_oid_to_string(&our_oid));
    offset += snprintf(commit_content + offset, 1024 - offset, "parent %s\n", bro_oid_to_string(&their_oid));
    offset += snprintf(commit_content + offset, 1024 - offset, "author %s %llu +0000\n", author, (unsigned long long)timestamp);
    offset += snprintf(commit_content + offset, 1024 - offset, "committer %s %llu +0000\n\n", author, (unsigned long long)timestamp);
    offset += snprintf(commit_content + offset, 1024 - offset, "Merge branch '%s'\n", branch);
    
    bro_oid commit_oid;
    bro_odb_write(commit_content, strlen(commit_content), &commit_oid, 3);
    free(commit_content);
    free(their_data);
    
    bro_refs_update(head_ref, &commit_oid);
    
    printf("Merge made by the 'fuse' strategy.\n");
    printf("Merge: %s..%s into %s\n", bro_oid_to_string(&our_oid), bro_oid_to_string(&their_oid), head_ref + 11);
    
    return 0;
}
