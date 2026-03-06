#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

int bro_deflate(const void *data, size_t len, void **out, size_t *out_len) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return -1;
    }

    zs.next_in = (Bytef *)data;
    zs.avail_in = len;

    int ret;
    char buf[16384];
    size_t total = 0;

    do {
        zs.next_out = (Bytef *)buf;
        zs.avail_out = sizeof(buf);
        ret = deflate(&zs, Z_FINISH);

        size_t produced = sizeof(buf) - zs.avail_out;
        char *tmp = realloc(*out, total + produced);
        if (!tmp) {
            free(*out);
            deflateEnd(&zs);
            return -1;
        }
        *out = tmp;
        memcpy((char *)*out + total, buf, produced);
        total += produced;
    } while (ret == Z_OK);

    deflateEnd(&zs);

    *out_len = total;
    return 0;
}

int bro_inflate(const void *data, size_t len, void **out, size_t *out_len) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (inflateInit2(&zs, -15) != Z_OK) {
        return -1;
    }

    zs.next_in = (Bytef *)data;
    zs.avail_in = len;

    int ret;
    char buf[16384];
    size_t total = 0;

    do {
        zs.next_out = (Bytef *)buf;
        zs.avail_out = sizeof(buf);
        ret = inflate(&zs, Z_NO_FLUSH);

        if (ret != Z_STREAM_END && ret != Z_OK) {
            free(*out);
            inflateEnd(&zs);
            return -1;
        }

        size_t produced = sizeof(buf) - zs.avail_out;
        char *tmp = realloc(*out, total + produced);
        if (!tmp) {
            free(*out);
            inflateEnd(&zs);
            return -1;
        }
        *out = tmp;
        memcpy((char *)*out + total, buf, produced);
        total += produced;
    } while (ret != Z_STREAM_END);

    inflateEnd(&zs);

    *out_len = total;
    return 0;
}
