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

int cmd_oopsie(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "oopsie: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *commit = "HEAD";
    if (argc > 1) {
        commit = argv[1];
    }
    
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        fprintf(stderr, "oopsie: not on a branch\n");
        return 1;
    }
    
    bro_oid commit_oid;
    if (strcmp(commit, "HEAD") == 0) {
        bro_refs_read(head_ref, &commit_oid);
    } else {
        for (int i = 0; i < 20 && i * 2 < (int)strlen(commit); i++) {
            unsigned int val;
            sscanf(commit + i * 2, "%02x", &val);
            commit_oid.sha1[i] = val;
        }
    }
    
    size_t size;
    int type;
    void *data = bro_odb_read(&commit_oid, &size, &type);
    if (!data || type != 3) {
        fprintf(stderr, "oopsie: invalid commit\n");
        free(data);
        return 1;
    }
    
    char *content = (char *)data;
    content[size] = 0;
    
    char *tree_line = strstr(content, "tree ");
    if (!tree_line) {
        fprintf(stderr, "oopsie: commit has no tree\n");
        free(data);
        return 1;
    }
    
    char tree_oid_str[41] = {0};
    char *end = strchr(tree_line + 5, '\n');
    if (end) strncpy(tree_oid_str, tree_line + 5, end - (tree_line + 5));
    else strncpy(tree_oid_str, tree_line + 5, 40);
    
    const char *author = getenv("BRO_AUTHOR");
    if (!author) author = "bro <bro@localhost>";
    
    uint64_t timestamp = time(NULL);
    
    bro_oid current_oid;
    bro_refs_read(head_ref, &current_oid);
    
    char *commit_content = malloc(1024);
    int offset = 0;
    
    offset += snprintf(commit_content + offset, 1024 - offset, "tree %s\n", tree_oid_str);
    offset += snprintf(commit_content + offset, 1024 - offset, "parent %s\n", bro_oid_to_string(&current_oid));
    offset += snprintf(commit_content + offset, 1024 - offset, "author %s %llu +0000\n", author, (unsigned long long)timestamp);
    offset += snprintf(commit_content + offset, 1024 - offset, "committer %s %llu +0000\n\n", author, (unsigned long long)timestamp);
    offset += snprintf(commit_content + offset, 1024 - offset, "Revert '%s'\n", commit);
    
    bro_oid new_oid;
    bro_odb_write(commit_content, strlen(commit_content), &new_oid, 3);
    free(commit_content);
    free(data);
    
    bro_refs_update(head_ref, &new_oid);
    
    printf("Reverted: %s\n", bro_oid_to_string(&commit_oid));
    printf("refs/heads/%s now at %s\n", head_ref + 11, bro_oid_to_string(&new_oid));
    
    return 0;
}
