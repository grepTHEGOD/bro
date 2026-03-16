#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <stdint.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern int bro_refs_update(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);
extern int bro_head_write(const char *refname);
extern int bro_odb_write(const void *data, size_t size, bro_oid *oid, int type);

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

static int compute_blob_oid(const char *path, char *oid_str) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);
    char header[64];
    int header_len = snprintf(header, sizeof(header), "%d %ld", 1, size);
    SHA1_CTX ctx; sha1_init(&ctx);
    sha1_update(&ctx, (unsigned char *)header, header_len + 1);
    sha1_update(&ctx, (unsigned char *)data, size);
    free(data);
    unsigned char digest[20]; sha1_final(digest, &ctx);
    for (int i = 0; i < 20; i++) sprintf(oid_str + i * 2, "%02x", digest[i]);
    oid_str[40] = 0;
    return 0;
}

int cmd_yeet(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro yeet -m <message>\n");
        return 1;
    }
    
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "yeet: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *message = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            message = argv[i + 1];
            break;
        }
    }
    
    if (!message) {
        fprintf(stderr, "yeet: commit message required. Use -m <message>\n");
        return 1;
    }
    
    FILE *f = fopen(".bro/index", "r");
    if (!f) {
        fprintf(stderr, "yeet: nothing to yeet. Run 'bro slap <file>' first.\n");
        return 1;
    }
    
    char **paths = NULL;
    char **oids = NULL;
    size_t count = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) < 41) continue;
        paths = realloc(paths, (count + 1) * sizeof(char *));
        oids = realloc(oids, (count + 1) * sizeof(char *));
        paths[count] = strdup(line + 41);
        oids[count] = strndup(line, 40);
        count++;
    }
    fclose(f);
    
    if (count == 0) {
        fprintf(stderr, "yeet: nothing to yeet. Run 'bro slap <file>' first.\n");
        return 0;
    }
    
    char *tree_content = malloc(1);
    tree_content[0] = 0;
    size_t tree_len = 0;
    
    for (size_t i = 0; i < count; i++) {
        char oid_str[41];
        if (compute_blob_oid(paths[i], oid_str) != 0) continue;
        
        size_t entry_len = strlen(paths[i]) + 48;
        tree_content = realloc(tree_content, tree_len + entry_len + 1);
        
        memcpy(tree_content + tree_len, "100644", 6);
        tree_len += 6;
        tree_content[tree_len++] = ' ';
        
        size_t path_len = strlen(paths[i]);
        memcpy(tree_content + tree_len, paths[i], path_len);
        tree_len += path_len;
        tree_content[tree_len++] = 0;
        
        memcpy(tree_content + tree_len, oid_str, 40);
        tree_len += 40;
        tree_content[tree_len] = 0;
    }
    
    bro_oid tree_oid;
    bro_odb_write(tree_content, tree_len, &tree_oid, 2);
    free(tree_content);
    
    const char *head_ref = bro_head_read();
    bro_oid parent_oid = {0};
    int has_parent = 0;
    if (head_ref && strncmp(head_ref, "refs/heads/", 11) == 0) {
        has_parent = bro_refs_read(head_ref, &parent_oid) == 0;
    }
    
    const char *author = getenv("BRO_AUTHOR");
    if (!author) author = "bro <bro@localhost>";
    
    char *commit_content = malloc(1024);
    int offset = 0;
    
    offset += snprintf(commit_content + offset, 1024 - offset, "tree %s\n", bro_oid_to_string(&tree_oid));
    
    if (has_parent) {
        offset += snprintf(commit_content + offset, 1024 - offset, "parent %s\n", bro_oid_to_string(&parent_oid));
    }
    
    uint64_t timestamp = time(NULL);
    offset += snprintf(commit_content + offset, 1024 - offset, "author %s %llu +0000\n", author, (unsigned long long)timestamp);
    offset += snprintf(commit_content + offset, 1024 - offset, "committer %s %llu +0000\n\n", author, (unsigned long long)timestamp);
    offset += snprintf(commit_content + offset, 1024 - offset, "%s\n", message);
    
    bro_oid commit_oid;
    bro_odb_write(commit_content, strlen(commit_content), &commit_oid, 3);
    free(commit_content);
    
    if (head_ref && strncmp(head_ref, "refs/heads/", 11) == 0) {
        bro_refs_update(head_ref, &commit_oid);
    }
    
    FILE *idx = fopen(".bro/index", "w");
    if (idx) fclose(idx);
    
    printf("[%s %s] %s\n", head_ref ? head_ref + 11 : "main", bro_oid_to_string(&commit_oid), message);
    
    for (size_t i = 0; i < count; i++) {
        free(paths[i]);
        free(oids[i]);
    }
    free(paths);
    free(oids);
    
    return 0;
}
