#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern char *bro_read_file(const char *path, size_t *len);
extern void *bro_odb_read(const bro_oid *oid, size_t *size, int *type);
extern int bro_inflate(const void *data, size_t len, void **out, size_t *out_len);
extern char *bro_odb_path(const bro_oid *oid);

static uint32_t rol(uint32_t val, int bits) { return (val << bits) | (val >> (32 - bits)); }
static uint32_t f(uint32_t t, uint32_t b, uint32_t c, uint32_t d) {
    if (t < 20) return (b & c) | (~b & d);
    if (t < 40) return b ^ c ^ d;
    if (t < 60) return (b & c) | (b & d) | (c & d);
    return b ^ c ^ d;
}
static uint32_t k(uint32_t t) {
    if (t < 20) return 0x5A827999;
    if (t < 40) return 0x6ED9EBA1;
    if (t < 60) return 0x8F1BBCDC;
    return 0xCA62C1D6;
}

typedef struct {
    uint32_t h[5];
    uint64_t msg_len;
    unsigned char block[64];
    size_t block_len;
} SHA1_CTX;

static void sha1_init(SHA1_CTX *ctx) {
    ctx->h[0] = 0x67452301; ctx->h[1] = 0xEFCDAB89; ctx->h[2] = 0x98BADCFE;
    ctx->h[3] = 0x10325476; ctx->h[4] = 0xC3D2E1F0;
    ctx->msg_len = 0; ctx->block_len = 0;
}

static void sha1_update(SHA1_CTX *ctx, const unsigned char *data, size_t len) {
    const unsigned char *p = data;
    size_t remaining;
    ctx->msg_len += len;
    if (ctx->block_len > 0) {
        remaining = 64 - ctx->block_len;
        if (len < remaining) { memcpy(ctx->block + ctx->block_len, p, len); ctx->block_len += len; return; }
        memcpy(ctx->block + ctx->block_len, p, remaining);
        ctx->block_len = 0; p += remaining; len -= remaining;
    }
    while (len >= 64) { memcpy(ctx->block, p, 64); uint32_t w[80];
        for (int i = 0; i < 16; i++) w[i] = (p[i*4] << 24) | (p[i*4+1] << 16) | (p[i*4+2] << 8) | p[i*4+3];
        for (int i = 16; i < 80; i++) w[i] = rol(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
        uint32_t a = ctx->h[0], b = ctx->h[1], c = ctx->h[2], d = ctx->h[3], e = ctx->h[4], temp;
        for (int i = 0; i < 80; i++) { temp = rol(a, 5) + f(i, b, c, d) + e + k(i) + w[i]; e = d; d = c; c = rol(b, 30); b = a; a = temp; }
        ctx->h[0] += a; ctx->h[1] += b; ctx->h[2] += c; ctx->h[3] += d; ctx->h[4] += e;
        p += 64; len -= 64;
    }
    if (len > 0) { memcpy(ctx->block, p, len); ctx->block_len = len; }
}

static void sha1_final(unsigned char *digest, SHA1_CTX *ctx) {
    unsigned char pad[64] = {0x80};
    size_t pad_len = (ctx->block_len < 56) ? (56 - ctx->block_len) : (64 + 56 - ctx->block_len);
    sha1_update(ctx, pad, pad_len);
    unsigned char len_bytes[8]; uint64_t bit_len = ctx->msg_len * 8;
    for (int i = 7; i >= 0; i--) { len_bytes[i] = bit_len & 0xFF; bit_len >>= 8; }
    sha1_update(ctx, len_bytes, 8);
    for (int i = 0; i < 5; i++) { digest[i*4] = (ctx->h[i] >> 24) & 0xFF; digest[i*4+1] = (ctx->h[i] >> 16) & 0xFF; digest[i*4+2] = (ctx->h[i] >> 8) & 0xFF; digest[i*4+3] = ctx->h[i] & 0xFF; }
}

static char *get_blob_content(const char *oid_str, size_t *out_len) {
    bro_oid oid;
    for (int i = 0; i < 20; i++) {
        unsigned int val;
        sscanf(oid_str + i * 2, "%02x", &val);
        oid.sha1[i] = val;
    }
    
    char *obj_path = bro_odb_path(&oid);
    FILE *f = fopen(obj_path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    void *compressed = malloc(size);
    fread(compressed, 1, size, f);
    fclose(f);
    
    void *uncompressed = NULL;
    size_t uncompressed_size;
    if (bro_inflate(compressed, size, &uncompressed, &uncompressed_size) < 0) {
        free(compressed);
        return NULL;
    }
    free(compressed);
    
    char *header_end = strchr(uncompressed, ' ');
    if (!header_end) { free(uncompressed); return NULL; }
    char *data_start = strchr(header_end + 1, '\0');
    if (!data_start) { free(uncompressed); return NULL; }
    data_start++;
    
    size_t data_len = uncompressed_size - (data_start - (char *)uncompressed);
    char *result = malloc(data_len + 1);
    memcpy(result, data_start, data_len);
    result[data_len] = 0;
    
    free(uncompressed);
    if (out_len) *out_len = data_len;
    return result;
}

static void diff_files(const char *file1, const char *file2) {
    size_t len1, len2;
    char *content1 = bro_read_file(file1, &len1);
    char *content2 = bro_read_file(file2, &len2);
    
    printf("diff --git a/%s b/%s\n", file1, file2);
    printf("--- a/%s\n", file1);
    printf("+++ b/%s\n", file2);
    
    if (!content1 && !content2) return;
    
    if (!content1) {
        printf("@@ -0,0 +1,%zu @@\n", len2);
        printf("+%s", content2);
    } else if (!content2) {
        printf("@@ -1,%zu +0,0 @@\n", len1);
        printf("-%s", content1);
    } else {
        char *l1 = content1;
        char *l2 = content2;
        int line = 1;
        while (l1 < content1 + len1 || l2 < content2 + len2) {
            char *next1 = strchr(l1, '\n');
            char *next2 = strchr(l2, '\n');
            
            int len1_line = next1 ? (next1 - l1) : strlen(l1);
            int len2_line = next2 ? (next2 - l2) : strlen(l2);
            
            if (len1_line != len2_line || (len1_line > 0 && strncmp(l1, l2, len1_line) != 0)) {
                printf("@@ %d %d %d %d @@\n", line, len1_line > 0 ? len1_line : 0, line, len2_line > 0 ? len2_line : 0);
                if (len1_line > 0) printf("-%.*s\n", len1_line, l1);
                if (len2_line > 0) printf("+%.*s\n", len2_line, l2);
            }
            
            if (next1) l1 = next1 + 1; else break;
            if (next2) l2 = next2 + 1; else break;
            line++;
        }
    }
    
    free(content1);
    free(content2);
}

int cmd_roast(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "roast: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    if (argc < 2) {
        FILE *f = fopen(".bro/index", "r");
        if (!f) {
            printf("no changes\n");
            return 0;
        }
        
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) < 41) continue;
            
            char oid_str[41] = {0};
            strncpy(oid_str, line, 40);
            char *path = line + 41;
            
            char work_oid[41];
            FILE *wf = fopen(path, "rb");
            if (!wf) continue;
            
            fseek(wf, 0, SEEK_END);
            long size = ftell(wf);
            fseek(wf, 0, SEEK_SET);
            
            char *data = malloc(size);
            fread(data, 1, size, wf);
            fclose(wf);
            
            char header[64];
            int header_len = snprintf(header, sizeof(header), "%d %ld", 1, size);
            SHA1_CTX ctx; sha1_init(&ctx);
            sha1_update(&ctx, (unsigned char *)header, header_len + 1);
            sha1_update(&ctx, (unsigned char *)data, size);
            free(data);
            unsigned char digest[20]; sha1_final(digest, &ctx);
            for (int i = 0; i < 20; i++) sprintf(work_oid + i * 2, "%02x", digest[i]);
            
            if (strcmp(oid_str, work_oid) != 0) {
                char *indexed_content = get_blob_content(oid_str, NULL);
                char *work_content = bro_read_file(path, NULL);
                
                if (indexed_content && work_content) {
                    diff_files("(cached)", path);
                } else if (!indexed_content && work_content) {
                    printf("diff --git a/%s b/%s\n", path, path);
                    printf("--- /dev/null\n");
                    printf("+++ b/%s\n", path);
                }
                
                free(indexed_content);
                free(work_content);
            }
        }
        fclose(f);
        return 0;
    }
    
    return 0;
}
