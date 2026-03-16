#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern int bro_file_exists(const char *path);
extern void *bro_odb_read(const bro_oid *oid, size_t *size, int *type);

void bro_oid_from_string(bro_oid *oid, const char *str);

int cmd_expose(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "expose: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *obj = "HEAD";
    if (argc > 1) {
        obj = argv[1];
    }
    
    bro_oid oid = {0};
    
    if (strcmp(obj, "HEAD") == 0) {
        FILE *f = fopen(".bro/HEAD", "r");
        if (f) {
            char ref[64];
            if (fgets(ref, sizeof(ref), f)) {
                if (strncmp(ref, "ref: ", 5) == 0) {
                    ref[strcspn(ref, "\n")] = 0;
                    char ref_path[256];
                    snprintf(ref_path, sizeof(ref_path), ".bro/%s", ref + 5);
                    fclose(f);
                    f = fopen(ref_path, "r");
                    if (f) {
                        char oid_str[64];
                        if (fgets(oid_str, sizeof(oid_str), f)) {
                            oid_str[strcspn(oid_str, "\n")] = 0;
                            bro_oid_from_string(&oid, oid_str);
                        }
                        fclose(f);
                    }
                }
            }
            fclose(f);
        }
    } else {
        bro_oid_from_string(&oid, obj);
    }
    
    size_t size;
    int type;
    void *data = bro_odb_read(&oid, &size, &type);
    if (!data) {
        fprintf(stderr, "expose: object not found\n");
        return 1;
    }
    
    char *content = (char *)data;
    content[size] = 0;
    
    fwrite(content, 1, size, stdout);
    
    free(data);
    
    return 0;
}
