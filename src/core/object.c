#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

static SHA1_CTX sha1_ctx;

void bro_hash_object(const void *data, size_t len, bro_oid *oid, int type) {
    char header[64];
    int header_len = snprintf(header, sizeof(header), "%d %zu", type, len);
    
    sha1_init(&sha1_ctx);
    sha1_update(&sha1_ctx, header, header_len + 1);
    sha1_update(&sha1_ctx, data, len);
    sha1_final(oid->sha1, &sha1_ctx);
}

bro_object *bro_object_read(const bro_oid *oid) {
    char path[64];
    char *repo_path = getenv("BRO_DIR");
    if (!repo_path) repo_path = ".bro";
    
    snprintf(path, sizeof(path), "%s/objects/%02x%02x%02x%02x%02x/%02x%02x%02x%02x%02x",
             repo_path,
             oid->sha1[0], oid->sha1[1], oid->sha1[2], oid->sha1[3], oid->sha1[4],
             oid->sha1[5], oid->sha1[6], oid->sha1[7], oid->sha1[8], oid->sha1[9]);

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *compressed = malloc(size);
    fread(compressed, 1, size, f);
    fclose(f);

    void *uncompressed = NULL;
    size_t uncompressed_len;
    bro_inflate(compressed, size, &uncompressed, &uncompressed_len);
    free(compressed);

    bro_object *obj = malloc(sizeof(bro_object));
    obj->data = uncompressed;
    obj->size = uncompressed_len;
    obj->type = ((char *)uncompressed)[0] - '0';

    return obj;
}

int bro_object_write(bro_object *obj, bro_oid *oid) {
    char header[64];
    int header_len = snprintf(header, sizeof(header), "%d %zu", obj->type, obj->size);
    header[header_len] = '\0';

    sha1_init(&sha1_ctx);
    sha1_update(&sha1_ctx, header, header_len + 1);
    sha1_update(&sha1_ctx, obj->data, obj->size);
    sha1_final(oid->sha1, &sha1_ctx);

    void *compressed = NULL;
    size_t compressed_len;
    bro_deflate(header, header_len + 1, &compressed, &compressed_len);
    bro_deflate(obj->data, obj->size, &compressed + compressed_len, &compressed_len);

    char *repo_path = getenv("BRO_DIR");
    if (!repo_path) repo_path = ".bro";
    
    char obj_path[64];
    snprintf(obj_path, sizeof(obj_path), "%s/objects/%02x%02x%02x%02x%02x",
             repo_path,
             oid->sha1[0], oid->sha1[1], oid->sha1[2], oid->sha1[3], oid->sha1[4]);

    mkdir(obj_path, 0755);
    snprintf(obj_path + strlen(obj_path), 64 - strlen(obj_path), "/%02x%02x%02x%02x%02x",
             oid->sha1[5], oid->sha1[6], oid->sha1[7], oid->sha1[8], oid->sha1[9]);

    FILE *f = fopen(obj_path, "wb");
    fwrite(compressed, 1, compressed_len, f);
    fclose(f);
    free(compressed);

    return 0;
}
