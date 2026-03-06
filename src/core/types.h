#ifndef BRO_TYPES_H
#define BRO_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#define BRO_SHA1_LEN 40
#define BRO_OBJ_BLOB 1
#define BRO_OBJ_TREE 2
#define BRO_OBJ_COMMIT 3
#define BRO_OBJ_TAG 4

typedef struct {
    unsigned char sha1[20];
} bro_oid;

typedef struct {
    uint8_t type;
    size_t size;
    void *data;
} bro_object;

typedef struct {
    char *path;
    mode_t mode;
    bro_oid oid;
} bro_tree_entry;

typedef struct {
    bro_tree_entry *entries;
    size_t count;
    size_t capacity;
} bro_tree;

typedef struct {
    bro_oid tree_oid;
    bro_oid parent_oid;
    char *author;
    char *message;
    uint64_t timestamp;
} bro_commit;

typedef struct {
    char *data;
    size_t len;
} bro_blob;

typedef struct {
    char *path;
    bool staged;
    bool tracked;
    bro_oid oid;
} bro_index_entry;

typedef struct {
    bro_index_entry *entries;
    size_t count;
    size_t capacity;
} bro_index;

typedef struct {
    char *name;
    bro_oid oid;
    bool is_branch;
} bro_ref;

typedef struct {
    char *name;
    char *url;
} bro_remote;

typedef struct {
    char *path;
    char *worktree;
    bro_remote *remotes;
    size_t remote_count;
} bro_repository;

char *bro_oid_to_string(const bro_oid *oid);
void bro_oid_from_string(bro_oid *oid, const char *str);
int bro_oid_cmp(const bro_oid *a, const bro_oid *b);

#endif
