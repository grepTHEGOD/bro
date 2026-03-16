#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdint.h>
#include "../core/types.h"

extern int bro_file_exists(const char *path);
extern int bro_is_dir(const char *path);
extern char *bro_read_file(const char *path, size_t *len);
extern int bro_refs_read(const char *refname, bro_oid *oid);
extern const char *bro_head_read(void);
extern void *bro_odb_read(const bro_oid *oid, size_t *size, int *type);

typedef struct {
    char path[256];
    char oid_str[41];
    int status;
} index_entry;

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

static int read_index(index_entry **entries, size_t *count) {
    *count = 0; *entries = NULL;
    FILE *f = fopen(".bro/index", "r");
    if (!f) return 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) < 41) continue;
        *entries = realloc(*entries, (*count + 1) * sizeof(index_entry));
        index_entry *e = &(*entries)[*count];
        strncpy(e->oid_str, line, 40); e->oid_str[40] = 0;
        strncpy(e->path, line + 41, 255); e->path[255] = 0;
        e->status = 0; (*count)++;
    }
    fclose(f);
    return 0;
}

static int get_file_oid(const char *path, char *oid_str) {
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

int cmd_vibe_check(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "vibe-check: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    const char *head_ref = bro_head_read();
    bro_oid head_oid = {0};
    int has_head = 0;
    if (head_ref && strncmp(head_ref, "refs/heads/", 11) == 0) {
        has_head = bro_refs_read(head_ref, &head_oid) == 0;
    }
    index_entry *indexed = NULL;
    size_t indexed_count = 0;
    read_index(&indexed, &indexed_count);
    printf("On branch ");
    if (head_ref && strncmp(head_ref, "refs/heads/", 11) == 0) printf("%s\n", head_ref + 11);
    else if (head_ref) printf("(detached)\n");
    else printf("(unknown)\n");
    if (has_head) printf("Last commit: %s\n", bro_oid_to_string(&head_oid));
    printf("\n");
    int staged_deletes = 0;
    int modified = 0;
    int untracked = 0;
    for (size_t i = 0; i < indexed_count; i++) {
        if (!bro_file_exists(indexed[i].path)) staged_deletes++;
    }
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.' || strcmp(entry->d_name, ".bro") == 0) continue;
            struct stat st;
            if (stat(entry->d_name, &st) != 0 || S_ISDIR(st.st_mode)) continue;
            char oid_str[41]; get_file_oid(entry->d_name, oid_str);
            int in_index = 0;
            for (size_t i = 0; i < indexed_count; i++) {
                if (strcmp(indexed[i].path, entry->d_name) == 0) {
                    in_index = 1;
                    if (strcmp(indexed[i].oid_str, oid_str) != 0) modified++;
                    break;
                }
            }
            if (!in_index) untracked++;
        }
        closedir(dir);
    }
    if (staged_deletes > 0 || modified > 0 || untracked > 0) {
        if (staged_deletes > 0) {
            printf("Changes to be committed:\n");
            for (size_t i = 0; i < indexed_count; i++) {
                if (!bro_file_exists(indexed[i].path)) printf("  deleted: %s\n", indexed[i].path);
            }
        }
        if (modified > 0 || untracked > 0) {
            printf("Changes not staged for commit:\n");
            dir = opendir(".");
            if (dir) {
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    if (entry->d_name[0] == '.' || strcmp(entry->d_name, ".bro") == 0) continue;
                    struct stat st;
                    if (stat(entry->d_name, &st) != 0 || S_ISDIR(st.st_mode)) continue;
                    char oid_str[41]; get_file_oid(entry->d_name, oid_str);
                    for (size_t i = 0; i < indexed_count; i++) {
                        if (strcmp(indexed[i].path, entry->d_name) == 0) {
                            if (strcmp(indexed[i].oid_str, oid_str) != 0) printf("  modified: %s\n", entry->d_name);
                            break;
                        }
                    }
                }
                closedir(dir);
            }
            if (untracked > 0) {
                printf("\nUntracked files:\n");
                dir = opendir(".");
                if (dir) {
                    struct dirent *entry;
                    while ((entry = readdir(dir)) != NULL) {
                        if (entry->d_name[0] == '.' || strcmp(entry->d_name, ".bro") == 0) continue;
                        struct stat st;
                        if (stat(entry->d_name, &st) != 0 || S_ISDIR(st.st_mode)) continue;
                        int in_index = 0;
                        for (size_t i = 0; i < indexed_count; i++) {
                            if (strcmp(indexed[i].path, entry->d_name) == 0) { in_index = 1; break; }
                        }
                        if (!in_index) printf("  %s\n", entry->d_name);
                    }
                    closedir(dir);
                }
            }
        }
    } else {
        printf("nothing to commit, vibe is clean ✨\n");
    }
    free(indexed);
    return 0;
}
