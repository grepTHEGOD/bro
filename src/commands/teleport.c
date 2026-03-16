#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern int bro_refs_update(const char *refname, bro_oid *oid);
extern int bro_head_write(const char *refname);
extern const char *bro_head_read(void);
extern void *bro_odb_read(const bro_oid *oid, size_t *size, int *type);
extern char *bro_odb_path(const bro_oid *oid);
extern int bro_inflate(const void *data, size_t len, void **out, size_t *out_len);

static int write_file(const char *path, const void *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(data, 1, len, f);
    fclose(f);
    return 0;
}

static void restore_tree(const char *tree_oid_str, const char *base_path) {
    bro_oid tree_oid;
    for (int i = 0; i < 20; i++) {
        unsigned int val;
        sscanf(tree_oid_str + i * 2, "%02x", &val);
        tree_oid.sha1[i] = val;
    }
    
    size_t size;
    int type;
    void *data = bro_odb_read(&tree_oid, &size, &type);
    if (!data || type != 2) { free(data); return; }
    
    char *content = (char *)data;
    content[size] = 0;
    
    char *p = content;
    while (p < content + size) {
        char *mode_end = strchr(p, ' ');
        if (!mode_end) break;
        *mode_end = 0;
        
        char *path_end = strchr(mode_end + 1, 0);
        if (!path_end) break;
        
        char *oid_start = path_end + 1;
        if (oid_start + 40 > content + size) break;
        
        char oid_str[41] = {0};
        strncpy(oid_str, oid_start, 40);
        
        char *path = mode_end + 1;
        
        if (strcmp(p, "100644") == 0 || strcmp(p, "100755") == 0) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", base_path, path);
            
            bro_oid blob_oid;
            for (int i = 0; i < 20; i++) {
                unsigned int val;
                sscanf(oid_str + i * 2, "%02x", &val);
                blob_oid.sha1[i] = val;
            }
            
            char *obj_path = bro_odb_path(&blob_oid);
            
            FILE *f = fopen(obj_path, "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                long blob_size = ftell(f);
                fseek(f, 0, SEEK_SET);
                
                void *compressed = malloc(blob_size);
                fread(compressed, 1, blob_size, f);
                fclose(f);
                
                void *uncompressed = NULL;
                size_t uncompressed_size;
                if (bro_inflate(compressed, blob_size, &uncompressed, &uncompressed_size) == 0) {
                    char *header_end = strchr(uncompressed, ' ');
                    if (header_end) {
                        char *data_start = strchr(header_end + 1, '\0');
                        if (data_start && data_start < (char *)uncompressed + uncompressed_size) {
                            data_start++;
                            size_t data_len = uncompressed_size - (data_start - (char *)uncompressed);
                            
                            char dir[512];
                            strcpy(dir, full_path);
                            char *last_slash = strrchr(dir, '/');
                            if (last_slash) {
                                *last_slash = 0;
                                mkdir(dir, 0755);
                            }
                            
                            write_file(full_path, data_start, data_len);
                        }
                    }
                    free(uncompressed);
                }
                free(compressed);
            }
        }
        
        p = oid_start + 40;
        while (p < content + size && *p) p++;
        p++;
    }
    
    free(data);
}

int cmd_teleport(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro teleport <branch|commit>\n");
        return 1;
    }
    
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "teleport: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *target = argv[1];
    bro_oid oid = {0};
    int is_branch = 0;
    char ref_name[256];
    
    snprintf(ref_name, sizeof(ref_name), "refs/heads/%s", target);
    if (bro_refs_read(ref_name, &oid) == 0) {
        is_branch = 1;
    } else {
        if (strlen(target) == 40) {
            for (int i = 0; i < 20; i++) {
                unsigned int val;
                sscanf(target + i * 2, "%02x", &val);
                oid.sha1[i] = val;
            }
        }
    }
    
    if (oid.sha1[0] == 0 && oid.sha1[1] == 0) {
        fprintf(stderr, "teleport: could not find '%s'\n", target);
        return 1;
    }
    
    size_t size;
    int type;
    void *data = bro_odb_read(&oid, &size, &type);
    if (!data || type != 3) {
        fprintf(stderr, "teleport: invalid commit\n");
        free(data);
        return 1;
    }
    
    char *content = (char *)data;
    content[size] = 0;
    
    char *tree_line = strstr(content, "tree ");
    if (!tree_line) {
        fprintf(stderr, "teleport: commit has no tree\n");
        free(data);
        return 1;
    }
    
    char tree_oid_str[41] = {0};
    char *end = strchr(tree_line + 5, '\n');
    if (end) {
        strncpy(tree_oid_str, tree_line + 5, end - (tree_line + 5));
    } else {
        strncpy(tree_oid_str, tree_line + 5, 40);
    }
    
    free(data);
    
    if (is_branch) {
        bro_head_write(ref_name);
        printf("Switched to squad '%s'\n", target);
    } else {
        char detached_ref[256];
        snprintf(detached_ref, sizeof(detached_ref), "%s", bro_oid_to_string(&oid));
        FILE *f = fopen(".bro/HEAD", "w");
        if (f) {
            fprintf(f, "%s\n", detached_ref);
            fclose(f);
        }
        printf("Note: checking out '%s'\nYou are in 'detached HEAD' state.\n", target);
    }
    
    restore_tree(tree_oid_str, ".");
    
    return 0;
}
