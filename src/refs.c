#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int bro_refs_update(const char *refname, bro_oid *oid) {
    char ref_path[512];
    snprintf(ref_path, sizeof(ref_path), ".bro/%s", refname);
    
    char *last_slash = strrchr(ref_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(ref_path, 0755);
        *last_slash = '/';
    }
    
    FILE *f = fopen(ref_path, "w");
    if (!f) return -1;
    
    fprintf(f, "%s\n", bro_oid_to_string(oid));
    fclose(f);
    
    return 0;
}

int bro_refs_read(const char *refname, bro_oid *oid) {
    char ref_path[512];
    snprintf(ref_path, sizeof(ref_path), ".bro/%s", refname);
    
    FILE *f = fopen(ref_path, "r");
    if (!f) return -1;
    
    char oid_str[64];
    if (!fgets(oid_str, sizeof(oid_str), f)) {
        fclose(f);
        return -1;
    }
    
    oid_str[strcspn(oid_str, "\n")] = '\0';
    bro_oid_from_string(oid, oid_str);
    
    fclose(f);
    return 0;
}

int bro_refs_delete(const char *refname) {
    char ref_path[512];
    snprintf(ref_path, sizeof(ref_path), ".bro/%s", refname);
    
    return unlink(ref_path);
}

int bro_refs_list(const char *prefix, bro_ref **refs, size_t *count) {
    char refs_dir[512];
    snprintf(refs_dir, sizeof(refs_dir), ".bro/%s", prefix);
    
    *count = 0;
    *refs = NULL;
    
    return 0;
}

const char *bro_head_read(void) {
    static char refname[256];
    
    FILE *f = fopen(".bro/HEAD", "r");
    if (!f) return NULL;
    
    if (!fgets(refname, sizeof(refname), f)) {
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    
    if (strncmp(refname, "ref: ", 5) == 0) {
        return refname + 5;
    }
    
    return NULL;
}

int bro_head_write(const char *refname) {
    FILE *f = fopen(".bro/HEAD", "w");
    if (!f) return -1;
    
    fprintf(f, "ref: %s\n", refname);
    fclose(f);
    
    return 0;
}

int bro_head_write_oid(bro_oid *oid) {
    FILE *f = fopen(".bro/HEAD", "w");
    if (!f) return -1;
    
    fprintf(f, "%s\n", bro_oid_to_string(oid));
    fclose(f);
    
    return 0;
}
