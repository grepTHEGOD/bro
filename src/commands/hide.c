#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern char *bro_read_file(const char *path, size_t *len);
extern int bro_mkdir(const char *path);
extern void bro_oid_from_string(bro_oid *oid, const char *str);

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

int cmd_hide(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "hide: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    bro_mkdir(".bro/refs/stash");
    
    char stash_file[256];
    snprintf(stash_file, sizeof(stash_file), ".bro/refs/stash/%lu", (unsigned long)time(NULL));
    
    FILE *sf = fopen(stash_file, "w");
    if (!sf) {
        fprintf(stderr, "hide: could not create stash\n");
        return 1;
    }
    
    FILE *idx = fopen(".bro/index", "r");
    if (idx) {
        char line[512];
        while (fgets(line, sizeof(line), idx)) {
            fputs(line, sf);
        }
        fclose(idx);
    }
    fclose(sf);
    
    idx = fopen(".bro/index", "w");
    if (idx) fclose(idx);
    
    printf("Saved working directory and index state\n");
    
    return 0;
}
