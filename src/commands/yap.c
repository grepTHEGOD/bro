#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);
extern void *bro_odb_read(const bro_oid *oid, size_t *size, int *type);
extern char *bro_time_format(uint64_t timestamp);
void bro_oid_from_string(bro_oid *oid, const char *str);

static void show_commit(const bro_oid *oid) {
    size_t size;
    int type;
    void *data = bro_odb_read(oid, &size, &type);
    if (!data || type != 3) { free(data); return; }
    
    char *content = (char *)data;
    content[size] = 0;
    
    char *tree_line = strstr(content, "tree ");
    char *author_line = strstr(content, "author ");
    char *msg_line = strstr(content, "\n\n");
    
    char tree_oid_str[41] = {0};
    if (tree_line) {
        char *end = strchr(tree_line + 5, '\n');
        if (end) {
            size_t len = end - (tree_line + 5);
            if (len < 40) {
                strncpy(tree_oid_str, tree_line + 5, len);
                tree_oid_str[len] = 0;
            }
        }
    }
    
    char author[128] = "Unknown";
    if (author_line) {
        char *end = strchr(author_line + 7, '<');
        if (end) {
            char *end2 = strchr(end, '>');
            if (end2 && end2 - end < 100) {
                strncpy(author, end + 1, end2 - end - 1);
                author[end2 - end - 1] = 0;
            }
        }
    }
    
    uint64_t timestamp = 0;
    if (author_line) {
        char *ts_start = strchr(author_line, '>');
        if (ts_start) {
            timestamp = strtoull(ts_start + 1, NULL, 10);
        }
    }
    
    printf("commit %s\n", bro_oid_to_string(oid));
    if (tree_line) printf("    tree %s\n", tree_oid_str);
    printf("Author: %s\n", author);
    printf("Date:   %s\n", bro_time_format(timestamp));
    
    if (msg_line) {
        printf("\n    %s\n", msg_line + 2);
    }
    printf("\n");
    
    free(data);
}

int cmd_yap(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "yap: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    int max_count = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            max_count = atoi(argv[++i]);
        }
    }
    
    bro_oid oid;
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        printf("No commits yet\n");
        return 0;
    }
    
    if (bro_refs_read(head_ref, &oid) != 0) {
        printf("No commits yet\n");
        return 0;
    }
    
    int count = 0;
    while (oid.sha1[0] || oid.sha1[1]) {
        if (max_count > 0 && count >= max_count) break;
        show_commit(&oid);
        count++;
        
        size_t size;
        int type;
        void *data = bro_odb_read(&oid, &size, &type);
        if (!data || type != 3) { free(data); break; }
        
        char *content = (char *)data;
        content[size] = 0;
        
        char *parent_line = strstr(content, "parent ");
        if (!parent_line) { free(data); break; }
        
        char *end = strchr(parent_line + 7, '\n');
        if (!end) { free(data); break; }
        
        size_t len = end - (parent_line + 7);
        if (len < 40 || len > 40) { free(data); break; }
        
        char parent_str[41] = {0};
        strncpy(parent_str, parent_line + 7, 40);
        
        free(data);
        
        bro_oid_from_string(&oid, parent_str);
        free(data);
    }
    
    return 0;
}
