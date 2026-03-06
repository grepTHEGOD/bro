#include "../core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t h[5];
    uint64_t msg_len;
    unsigned char block[64];
    size_t block_len;
} SHA1_CTX;

void sha1_init(SHA1_CTX *ctx);
void sha1_update(SHA1_CTX *ctx, const void *data, size_t len);
void sha1_final(unsigned char *digest, SHA1_CTX *ctx);
void bro_oid_from_string(bro_oid *oid, const char *str);
char *bro_oid_to_string(const bro_oid *oid);

int bro_inflate(const void *data, size_t len, void **out, size_t *out_len);

int cmd_peek(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro peek <object>\n");
        return 1;
    }
    
    const char *oid_str = argv[1];
    bro_oid oid;
    bro_oid_from_string(&oid, oid_str);
    
    char path[64];
    char *repo_path = getenv("BRO_DIR");
    if (!repo_path) repo_path = ".bro";
    
    snprintf(path, sizeof(path), "%s/objects/%02x%02x%02x%02x%02x/%02x%02x%02x%02x%02x",
             repo_path,
             oid.sha1[0], oid.sha1[1], oid.sha1[2], oid.sha1[3], oid.sha1[4],
             oid.sha1[5], oid.sha1[6], oid.sha1[7], oid.sha1[8], oid.sha1[9]);
    
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "bro: object %s not found\n", oid_str);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    void *compressed = malloc(size);
    fread(compressed, 1, size, f);
    fclose(f);
    
    void *uncompressed = NULL;
    size_t uncompressed_size;
    if (bro_inflate(compressed, size, &uncompressed, &uncompressed_size) < 0 || !uncompressed) {
        free(compressed);
        fprintf(stderr, "bro: failed to decompress object\n");
        return 1;
    }
    free(compressed);
    
    int type = ((char*)uncompressed)[0] - '0';
    char *content = (char *)uncompressed;
    
    while (*content && *content != ' ') content++;
    while (*content && *content == ' ') content++;
    char *space = strchr(content, ' ');
    if (space) *space = '\0';
    
    printf("type: %d\n", type);
    printf("size: %zu\n", uncompressed_size);
    printf("\n");
    fwrite(uncompressed, 1, uncompressed_size, stdout);
    
    free(uncompressed);
    return 0;
}
