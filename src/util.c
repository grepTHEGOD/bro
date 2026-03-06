#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

char *bro_strdup(const char *s) {
    return strdup(s);
}

char *bro_strjoin(const char *a, const char *b) {
    size_t len = strlen(a) + strlen(b) + 1;
    char *result = malloc(len);
    snprintf(result, len, "%s%s", a, b);
    return result;
}

int bro_file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

int bro_is_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

int bro_is_file(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

char *bro_read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *data = malloc(size + 1);
    fread(data, 1, size, f);
    data[size] = '\0';
    fclose(f);
    
    if (len) *len = size;
    return data;
}

int bro_write_file(const char *path, const void *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    
    fwrite(data, 1, len, f);
    fclose(f);
    
    return 0;
}

int bro_mkdir(const char *path) {
    return mkdir(path, 0755);
}

int bro_rm(const char *path) {
    return unlink(path);
}

int bro_cp(const char *src, const char *dst) {
    size_t len;
    char *data = bro_read_file(src, &len);
    if (!data) return -1;
    
    int ret = bro_write_file(dst, data, len);
    free(data);
    return ret;
}

char *bro_basename(const char *path) {
    const char *base = strrchr(path, '/');
    return base ? (char *)(base + 1) : (char *)path;
}

char *bro_dirname(const char *path) {
    static char dir[512];
    strcpy(dir, path);
    
    char *last_slash = strrchr(dir, '/');
    if (last_slash) {
        *last_slash = '\0';
    } else {
        strcpy(dir, ".");
    }
    
    return dir;
}

char *bro_time_format(uint64_t timestamp) {
    static char buf[64];
    time_t t = timestamp;
    struct tm *tm = localtime(&t);
    strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", tm);
    return buf;
}

int bro_path_normalize(char *path) {
    char *src = path;
    char *dst = path;
    char comp[256];
    
    while (*src) {
        if (*src == '/') {
            *dst++ = '/';
            src++;
            while (*src == '/') src++;
        } else if (*src == '.' && src[1] == '/' ) {
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }
    
    *dst = '\0';
    return 0;
}

int bro_diff_files(const char *file1, const char *file2) {
    printf("diff --git a/%s b/%s\n", file1, file2);
    printf("--- a/%s\n", file1);
    printf("+++ b/%s\n", file2);
    return 0;
}