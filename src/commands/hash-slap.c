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

int cmd_hash_slap(int argc, char **argv) {
    int type = 1;
    int read_stdin = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            type = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--stdin") == 0) {
            read_stdin = 1;
        }
    }
    
    if (read_stdin) {
        char buffer[8192];
        size_t total = 0;
        
        SHA1_CTX ctx;
        sha1_init(&ctx);
        
        while (fgets(buffer, sizeof(buffer), stdin)) {
            size_t len = strlen(buffer);
            sha1_update(&ctx, buffer, len);
            total += len;
        }
        
        unsigned char digest[20];
        sha1_final(digest, &ctx);
        
        bro_oid oid;
        memcpy(oid.sha1, digest, 20);
        
        printf("%s\n", bro_oid_to_string(&oid));
    } else {
        const char *data = "";
        size_t len = 0;
        
        if (argc > 1) {
            data = argv[1];
            len = strlen(data);
        }
        
        char header[64];
        int header_len = snprintf(header, sizeof(header), "%d %zu", type, len);
        
        SHA1_CTX ctx;
        sha1_init(&ctx);
        sha1_update(&ctx, header, header_len + 1);
        sha1_update(&ctx, data, len);
        
        unsigned char digest[20];
        sha1_final(digest, &ctx);
        
        bro_oid oid;
        memcpy(oid.sha1, digest, 20);
        
        printf("%s\n", bro_oid_to_string(&oid));
    }
    
    return 0;
}
