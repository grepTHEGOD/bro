#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern int bro_is_dir(const char *path);
extern char *bro_read_file(const char *path, size_t *len);
extern int bro_mkdir(const char *path);
extern void bro_oid_from_string(bro_oid *oid, const char *str);
extern char *bro_oid_to_string(const bro_oid *oid);

typedef struct {
    uint32_t h[5];
    uint64_t msg_len;
    unsigned char block[64];
    size_t block_len;
} SHA1_CTX;

static void local_sha1_init(SHA1_CTX *ctx);
static void local_sha1_update(SHA1_CTX *ctx, const unsigned char *data, size_t len);
static void local_sha1_final(unsigned char *digest, SHA1_CTX *ctx);

static void local_sha1_init(SHA1_CTX *ctx) {
    ctx->h[0] = 0x67452301;
    ctx->h[1] = 0xEFCDAB89;
    ctx->h[2] = 0x98BADCFE;
    ctx->h[3] = 0x10325476;
    ctx->h[4] = 0xC3D2E1F0;
    ctx->msg_len = 0;
    ctx->block_len = 0;
}

static void local_sha1_update(SHA1_CTX *ctx, const unsigned char *data, size_t len) {
    ctx->msg_len += len;
    size_t remaining;
    
    if (ctx->block_len > 0) {
        remaining = 64 - ctx->block_len;
        if (len < remaining) {
            memcpy(ctx->block + ctx->block_len, data, len);
            ctx->block_len += len;
            return;
        }
        memcpy(ctx->block + ctx->block_len, data, remaining);
        ctx->block_len = 0;
        data += remaining;
        len -= remaining;
    }
    
    while (len >= 64) {
        memcpy(ctx->block, data, 64);
        data += 64;
        len -= 64;
    }
    
    if (len > 0) {
        memcpy(ctx->block, data, len);
        ctx->block_len = len;
    }
}

static void local_sha1_final(unsigned char *digest, SHA1_CTX *ctx) {
    unsigned char pad[64] = {0x80};
    size_t pad_len = (ctx->block_len < 56) ? (56 - ctx->block_len) : (64 + 56 - ctx->block_len);
    local_sha1_update(ctx, pad, pad_len);
    
    unsigned char len_bytes[8];
    uint64_t bit_len = ctx->msg_len * 8;
    for (int i = 7; i >= 0; i--) {
        len_bytes[i] = bit_len & 0xFF;
        bit_len >>= 8;
    }
    local_sha1_update(ctx, len_bytes, 8);
    
    for (int i = 0; i < 5; i++) {
        digest[i * 4] = (ctx->h[i] >> 24) & 0xFF;
        digest[i * 4 + 1] = (ctx->h[i] >> 16) & 0xFF;
        digest[i * 4 + 2] = (ctx->h[i] >> 8) & 0xFF;
        digest[i * 4 + 3] = ctx->h[i] & 0xFF;
    }
}

static int write_blob(const char *path, bro_oid *oid) {
    size_t len;
    char *data = bro_read_file(path, &len);
    if (!data) {
        fprintf(stderr, "slap: could not read %s\n", path);
        return -1;
    }

    char header[64];
    int header_len = snprintf(header, sizeof(header), "%d %zu", 1, len);
    header[header_len] = '\0';

    SHA1_CTX ctx;
    local_sha1_init(&ctx);
    local_sha1_update(&ctx, (const unsigned char *)header, header_len + 1);
    local_sha1_update(&ctx, (const unsigned char *)data, len);
    local_sha1_final(oid->sha1, &ctx);

    char obj_path[256];
    snprintf(obj_path, sizeof(obj_path), ".bro/objects/%02x%02x%02x%02x%02x",
             oid->sha1[0], oid->sha1[1], oid->sha1[2], oid->sha1[3], oid->sha1[4]);
    bro_mkdir(obj_path);
    snprintf(obj_path + strlen(obj_path), 64, "/%02x%02x%02x%02x%02x",
             oid->sha1[5], oid->sha1[6], oid->sha1[7], oid->sha1[8], oid->sha1[9]);

    FILE *f = fopen(obj_path, "wb");
    if (!f) {
        free(data);
        return -1;
    }
    fwrite(data, 1, len, f);
    fclose(f);
    free(data);

    return 0;
}

int cmd_slap(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: bro slap <file>...\n");
        return 0;
    }

    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "slap: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char *path = argv[i];

        if (bro_is_dir(path)) {
            printf("slap: skipping directory %s\n", path);
            continue;
        }

        bro_oid oid;
        if (write_blob(path, &oid) == 0) {
            char index_path[256];
            snprintf(index_path, sizeof(index_path), ".bro/index");

            FILE *idx = fopen(index_path, "a");
            if (idx) {
                fprintf(idx, "%s %s\n", bro_oid_to_string(&oid), path);
                fclose(idx);
            }
        }
    }

    return 0;
}
