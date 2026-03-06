#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

typedef struct {
    uint32_t h[5];
    uint64_t msg_len;
    unsigned char block[64];
    size_t block_len;
} SHA1_CTX;

void sha1_init(SHA1_CTX *ctx);
void sha1_update(SHA1_CTX *ctx, const void *data, size_t len);
void sha1_final(unsigned char *digest, SHA1_CTX *ctx);
int bro_deflate(const void *data, size_t len, void **out, size_t *out_len);
int bro_inflate(const void *data, size_t len, void **out, size_t *out_len);

char *bro_odb_path(const bro_oid *oid) {
    static char path[128];
    char *repo_path = getenv("BRO_DIR");
    if (!repo_path) repo_path = ".bro";
    
    snprintf(path, sizeof(path), "%s/objects/%02x%02x%02x%02x%02x/%02x%02x%02x%02x%02x",
             repo_path,
             oid->sha1[0], oid->sha1[1], oid->sha1[2], oid->sha1[3], oid->sha1[4],
             oid->sha1[5], oid->sha1[6], oid->sha1[7], oid->sha1[8], oid->sha1[9]);
    return path;
}

int bro_odb_exists(const bro_oid *oid) {
    char *path = bro_odb_path(oid);
    struct stat st;
    return stat(path, &st) == 0;
}

int bro_odb_write(const void *data, size_t size, bro_oid *oid, int type) {
    char header[64];
    int header_len = snprintf(header, sizeof(header), "%d %zu", type, size);
    
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, header, header_len + 1);
    sha1_update(&ctx, data, size);
    sha1_final(oid->sha1, &ctx);
    
    if (bro_odb_exists(oid)) {
        return 0;
    }
    
    void *compressed = NULL;
    size_t compressed_len;
    
    size_t total_len = header_len + 1 + size;
    char *total = malloc(total_len);
    memcpy(total, header, header_len + 1);
    memcpy(total + header_len + 1, data, size);
    
    bro_deflate(total, total_len, &compressed, &compressed_len);
    free(total);
    
    char *path = bro_odb_path(oid);
    char *dir = strdup(path);
    char *last_slash = strrchr(dir, '/');
    *last_slash = '\0';
    
    mkdir(dir, 0755);
    free(dir);
    
    FILE *f = fopen(path, "wb");
    fwrite(compressed, 1, compressed_len, f);
    fclose(f);
    free(compressed);
    
    return 0;
}

void *bro_odb_read(const bro_oid *oid, size_t *size, int *type) {
    char *path = bro_odb_path(oid);
    
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    void *compressed = malloc(fsize);
    fread(compressed, 1, fsize, f);
    fclose(f);
    
    void *uncompressed = NULL;
    size_t uncompressed_size;
    if (bro_inflate(compressed, fsize, &uncompressed, &uncompressed_size) < 0) {
        free(compressed);
        return NULL;
    }
    free(compressed);
    
    if (size) *size = uncompressed_size;
    
    if (type) {
        *type = ((char*)uncompressed)[0] - '0';
    }
    
    return uncompressed;
}

int bro_odb_foreach(void (*callback)(const bro_oid *oid, void *data), void *arg) {
    char *repo_path = getenv("BRO_DIR");
    if (!repo_path) repo_path = ".bro";
    
    char objects_dir[128];
    snprintf(objects_dir, sizeof(objects_dir), "%s/objects", repo_path);
    
    DIR *dir = opendir(objects_dir);
    if (!dir) return -1;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (strlen(entry->d_name) != 2) continue;
        
        char subdir[128];
        snprintf(subdir, sizeof(subdir), "%s/%s", objects_dir, entry->d_name);
        
        DIR *sub = opendir(subdir);
        if (!sub) continue;
        
        struct dirent *file;
        while ((file = readdir(sub)) != NULL) {
            if (file->d_name[0] == '.') continue;
            
            char full_path[256];
            snprintf(full_path, sizeof(full_path), "%s/%s", subdir, file->d_name);
            
            bro_oid oid;
            char prefix[3] = {entry->d_name[0], entry->d_name[1], '\0'};
            unsigned int val;
            sscanf(prefix, "%02x", &val);
            oid.sha1[0] = val;
            
            for (int i = 0; i < 5; i++) {
                char chunk[3] = {file->d_name[i*2], file->d_name[i*2+1], '\0'};
                sscanf(chunk, "%02x", &val);
                oid.sha1[i+1] = val;
            }
            
            callback(&oid, arg);
        }
        closedir(sub);
    }
    closedir(dir);
    
    return 0;
}
