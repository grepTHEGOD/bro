#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

bro_index *bro_index_create(void) {
    bro_index *index = calloc(1, sizeof(bro_index));
    return index;
}

void bro_index_free(bro_index *index) {
    if (!index) return;
    for (size_t i = 0; i < index->count; i++) {
        free(index->entries[i].path);
    }
    free(index->entries);
    free(index);
}

int bro_index_add(bro_index *index, const char *path, bro_oid *oid, bool staged) {
    for (size_t i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            index->entries[i].oid = *oid;
            index->entries[i].staged = staged;
            return 0;
        }
    }
    
    if (index->count >= index->capacity) {
        index->capacity = index->capacity ? index->capacity * 2 : 32;
        index->entries = realloc(index->entries, index->capacity * sizeof(bro_index_entry));
    }
    
    index->entries[index->count].path = strdup(path);
    index->entries[index->count].oid = *oid;
    index->entries[index->count].staged = staged;
    index->entries[index->count].tracked = true;
    index->count++;
    
    return 0;
}

int bro_index_remove(bro_index *index, const char *path) {
    for (size_t i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            free(index->entries[i].path);
            for (size_t j = i; j < index->count - 1; j++) {
                index->entries[j] = index->entries[j + 1];
            }
            index->count--;
            return 0;
        }
    }
    return -1;
}

bro_index_entry *bro_index_lookup(bro_index *index, const char *path) {
    for (size_t i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            return &index->entries[i];
        }
    }
    return NULL;
}

int bro_index_read(bro_index *index) {
    FILE *f = fopen(".bro/index", "rb");
    if (!f) return -1;
    
    char magic[4];
    fread(magic, 1, 4, f);
    if (memcmp(magic, "BROI", 4) != 0) {
        fclose(f);
        return -1;
    }
    
    uint32_t count;
    fread(&count, 4, 1, f);
    
    for (uint32_t i = 0; i < count; i++) {
        uint16_t path_len;
        fread(&path_len, 2, 1, f);
        
        char *path = malloc(path_len + 1);
        fread(path, 1, path_len, f);
        path[path_len] = '\0';
        
        bro_oid oid;
        fread(oid.sha1, 1, 20, f);
        
        uint8_t flags;
        fread(&flags, 1, 1, f);
        
        bro_index_add(index, path, &oid, flags & 1);
        free(path);
    }
    
    fclose(f);
    return 0;
}

int bro_index_write(bro_index *index) {
    FILE *f = fopen(".bro/index", "wb");
    if (!f) return -1;
    
    fwrite("BROI", 1, 4, f);
    
    uint32_t count = index->count;
    fwrite(&count, 4, 1, f);
    
    for (size_t i = 0; i < index->count; i++) {
        bro_index_entry *e = &index->entries[i];
        
        uint16_t path_len = strlen(e->path);
        fwrite(&path_len, 2, 1, f);
        fwrite(e->path, 1, path_len, f);
        fwrite(e->oid.sha1, 1, 20, f);
        
        uint8_t flags = e->staged ? 1 : 0;
        fwrite(&flags, 1, 1, f);
    }
    
    fclose(f);
    return 0;
}
