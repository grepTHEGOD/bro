#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);
extern void *bro_odb_read(const bro_oid *oid, size_t *size, int *type);

int cmd_soft_undo(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "soft-undo: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        fprintf(stderr, "soft-undo: not on a branch\n");
        return 1;
    }
    
    bro_oid oid = {0};
    if (bro_refs_read(head_ref, &oid) != 0) {
        fprintf(stderr, "soft-undo: no commits\n");
        return 1;
    }
    
    size_t size;
    int type;
    void *data = bro_odb_read(&oid, &size, &type);
    if (!data || type != 3) {
        fprintf(stderr, "soft-undo: invalid commit\n");
        free(data);
        return 1;
    }
    
    char *content = (char *)data;
    content[size] = 0;
    
    char *parent_line = strstr(content, "parent ");
    if (!parent_line) {
        fprintf(stderr, "soft-undo: already at first commit\n");
        free(data);
        return 1;
    }
    
    char *end = strchr(parent_line + 7, '\n');
    if (!end) {
        free(data);
        return 1;
    }
    
    char parent_str[41] = {0};
    strncpy(parent_str, parent_line + 7, end - (parent_line + 7));
    
    free(data);
    
    FILE *f = fopen(".bro/index", "r");
    if (f) fclose(f);
    
    printf("HEAD is now at %s\n", parent_str);
    
    return 0;
}
