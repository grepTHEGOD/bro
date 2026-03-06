#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bro_tree *bro_tree_create(void) {
    bro_tree *tree = calloc(1, sizeof(bro_tree));
    return tree;
}

void bro_tree_free(bro_tree *tree) {
    if (!tree) return;
    for (size_t i = 0; i < tree->count; i++) {
        free(tree->entries[i].path);
    }
    free(tree->entries);
    free(tree);
}

int bro_tree_add_entry(bro_tree *tree, const char *path, bro_oid *oid, mode_t mode) {
    if (tree->count >= tree->capacity) {
        tree->capacity = tree->capacity ? tree->capacity * 2 : 16;
        tree->entries = realloc(tree->entries, tree->capacity * sizeof(bro_tree_entry));
    }
    tree->entries[tree->count].path = strdup(path);
    tree->entries[tree->count].oid = *oid;
    tree->entries[tree->count].mode = mode;
    tree->count++;
    return 0;
}

bro_tree_entry *bro_tree_get_entry(bro_tree *tree, const char *path) {
    for (size_t i = 0; i < tree->count; i++) {
        if (strcmp(tree->entries[i].path, path) == 0) {
            return &tree->entries[i];
        }
    }
    return NULL;
}

char *bro_tree_serialize(bro_tree *tree, size_t *len) {
    size_t capacity = 1024;
    char *buf = malloc(capacity);
    size_t offset = 0;

    for (size_t i = 0; i < tree->count; i++) {
        bro_tree_entry *e = &tree->entries[i];
        char entry[256];
        int entry_len = snprintf(entry, sizeof(entry), "%o %s", e->mode, e->path);
        
        if (offset + entry_len + 21 + 1 > capacity) {
            capacity *= 2;
            buf = realloc(buf, capacity);
        }
        
        memcpy(buf + offset, entry, entry_len);
        offset += entry_len;
        buf[offset++] = '\0';
        memcpy(buf + offset, e->oid.sha1, 20);
        offset += 20;
    }

    *len = offset;
    return buf;
}

bro_tree *bro_tree_deserialize(const char *data, size_t len) {
    bro_tree *tree = bro_tree_create();
    size_t offset = 0;

    while (offset < len) {
        char *space = strchr(data + offset, ' ');
        if (!space) break;
        
        mode_t mode = atoi(data + offset);
        char *path = space + 1;
        char *null = strchr(path, '\0');
        
        bro_oid oid;
        memcpy(oid.sha1, null + 1, 20);
        
        bro_tree_add_entry(tree, path, &oid, mode);
        
        offset = (null + 1 + 20) - data;
    }

    return tree;
}
