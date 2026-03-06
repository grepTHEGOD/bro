#include "types.h"
#include <string.h>
#include <stdio.h>

#define SHA1_BLOCK_SIZE 64
#define SHA1_DIGEST_SIZE 20

typedef struct {
    uint32_t h[5];
    uint64_t msg_len;
    unsigned char block[SHA1_BLOCK_SIZE];
    size_t block_len;
} SHA1_CTX;

static uint32_t rol(uint32_t val, int bits) {
    return (val << bits) | (val >> (32 - bits));
}

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

static void sha1_block(SHA1_CTX *ctx, const unsigned char *block) {
    uint32_t w[80];
    uint32_t a, b, c, d, e, temp;
    int t;

    for (t = 0; t < 16; t++) {
        w[t] = (block[t * 4] << 24) | (block[t * 4 + 1] << 16) |
               (block[t * 4 + 2] << 8) | block[t * 4 + 3];
    }

    for (t = 16; t < 80; t++) {
        uint32_t val = w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16];
        w[t] = rol(val, 1);
    }

    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];

    for (t = 0; t < 80; t++) {
        temp = rol(a, 5) + f(t, b, c, d) + e + k(t) + w[t];
        e = d;
        d = c;
        c = rol(b, 30);
        b = a;
        a = temp;
    }

    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
}

void sha1_init(SHA1_CTX *ctx) {
    ctx->h[0] = 0x67452301;
    ctx->h[1] = 0xEFCDAB89;
    ctx->h[2] = 0x98BADCFE;
    ctx->h[3] = 0x10325476;
    ctx->h[4] = 0xC3D2E1F0;
    ctx->msg_len = 0;
    ctx->block_len = 0;
}

void sha1_update(SHA1_CTX *ctx, const void *data, size_t len) {
    const unsigned char *p = data;
    size_t remaining;

    ctx->msg_len += len;

    if (ctx->block_len > 0) {
        remaining = SHA1_BLOCK_SIZE - ctx->block_len;
        if (len < remaining) {
            memcpy(ctx->block + ctx->block_len, p, len);
            ctx->block_len += len;
            return;
        }
        memcpy(ctx->block + ctx->block_len, p, remaining);
        sha1_block(ctx, ctx->block);
        ctx->block_len = 0;
        p += remaining;
        len -= remaining;
    }

    while (len >= SHA1_BLOCK_SIZE) {
        sha1_block(ctx, p);
        p += SHA1_BLOCK_SIZE;
        len -= SHA1_BLOCK_SIZE;
    }

    if (len > 0) {
        memcpy(ctx->block, p, len);
        ctx->block_len = len;
    }
}

void sha1_final(unsigned char *digest, SHA1_CTX *ctx) {
    uint64_t bit_len = ctx->msg_len * 8;
    unsigned char pad[SHA1_BLOCK_SIZE] = {0x80};
    size_t pad_len = (ctx->block_len < 56) ? (56 - ctx->block_len) :
                     (SHA1_BLOCK_SIZE + 56 - ctx->block_len);

    sha1_update(ctx, pad, pad_len);

    unsigned char len_bytes[8];
    for (int i = 7; i >= 0; i--) {
        len_bytes[i] = bit_len & 0xFF;
        bit_len >>= 8;
    }
    sha1_update(ctx, len_bytes, 8);

    for (int i = 0; i < 5; i++) {
        digest[i * 4] = (ctx->h[i] >> 24) & 0xFF;
        digest[i * 4 + 1] = (ctx->h[i] >> 16) & 0xFF;
        digest[i * 4 + 2] = (ctx->h[i] >> 8) & 0xFF;
        digest[i * 4 + 3] = ctx->h[i] & 0xFF;
    }
}

void bro_oid_from_string(bro_oid *oid, const char *str) {
    for (int i = 0; i < 20; i++) {
        unsigned int val;
        sscanf(str + i * 2, "%02x", &val);
        oid->sha1[i] = val;
    }
}

char *bro_oid_to_string(const bro_oid *oid) {
    static char buf[BRO_SHA1_LEN + 1];
    for (int i = 0; i < 20; i++) {
        sprintf(buf + i * 2, "%02x", oid->sha1[i]);
    }
    buf[BRO_SHA1_LEN] = '\0';
    return buf;
}

int bro_oid_cmp(const bro_oid *a, const bro_oid *b) {
    return memcmp(a->sha1, b->sha1, 20);
}
