#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern int bro_refs_update(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);
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

static void restore_tree(const char *tree_oid_str, const char *base_path);

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
                            
                            FILE *wf = fopen(full_path, "wb");
                            if (wf) {
                                fwrite(data_start, 1, data_len, wf);
                                fclose(wf);
                            }
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

int cmd_hard_undo(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "hard-undo: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        fprintf(stderr, "hard-undo: not on a branch\n");
        return 1;
    }
    
    bro_oid oid = {0};
    if (bro_refs_read(head_ref, &oid) != 0) {
        fprintf(stderr, "hard-undo: no commits\n");
        return 1;
    }
    
    size_t size;
    int type;
    void *data = bro_odb_read(&oid, &size, &type);
    if (!data || type != 3) {
        fprintf(stderr, "hard-undo: invalid commit\n");
        free(data);
        return 1;
    }
    
    char *content = (char *)data;
    content[size] = 0;
    
    char *tree_line = strstr(content, "tree ");
    if (!tree_line) {
        fprintf(stderr, "hard-undo: commit has no tree\n");
        free(data);
        return 1;
    }
    
    char tree_oid_str[41] = {0};
    char *end = strchr(tree_line + 5, '\n');
    if (end) strncpy(tree_oid_str, tree_line + 5, end - (tree_line + 5));
    else strncpy(tree_oid_str, tree_line + 5, 40);
    
    free(data);
    
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (strcmp(entry->d_name, ".bro") == 0) continue;
            unlink(entry->d_name);
        }
        closedir(dir);
    }
    
    FILE *idx = fopen(".bro/index", "w");
    if (idx) fclose(idx);
    
    restore_tree(tree_oid_str, ".");
    
    printf("HEAD is now at %s\n", bro_oid_to_string(&oid));
    
    return 0;
}
