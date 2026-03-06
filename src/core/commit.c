#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

bro_object *bro_object_read(const bro_oid *oid);
int bro_object_write(bro_object *obj, bro_oid *oid);

bro_commit *bro_commit_create(bro_oid *tree_oid, bro_oid *parent_oid, 
                               const char *author, const char *message) {
    bro_commit *commit = calloc(1, sizeof(bro_commit));
    commit->tree_oid = *tree_oid;
    if (parent_oid) {
        commit->parent_oid = *parent_oid;
    }
    commit->author = strdup(author);
    commit->message = strdup(message);
    commit->timestamp = time(NULL);
    return commit;
}

void bro_commit_free(bro_commit *commit) {
    if (!commit) return;
    free(commit->author);
    free(commit->message);
    free(commit);
}

char *bro_commit_serialize(bro_commit *commit, size_t *len) {
    char *buf = malloc(4096);
    int offset = 0;
    
    offset += snprintf(buf + offset, 4096 - offset, "tree %s\n", 
                       bro_oid_to_string(&commit->tree_oid));
    
    if (memcmp(commit->parent_oid.sha1, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 20) != 0) {
        offset += snprintf(buf + offset, 4096 - offset, "parent %s\n",
                           bro_oid_to_string(&commit->parent_oid));
    }
    
    offset += snprintf(buf + offset, 4096 - offset, "author %s\n", commit->author);
    offset += snprintf(buf + offset, 4096 - offset, "committer %s\n", commit->author);
    offset += snprintf(buf + offset, 4096 - offset, "timestamp %lu\n", commit->timestamp);
    offset += snprintf(buf + offset, 4096 - offset, "\n%s\n", commit->message);
    
    *len = offset;
    return buf;
}

bro_commit *bro_commit_deserialize(const char *data, size_t len) {
    bro_commit *commit = calloc(1, sizeof(bro_commit));
    char *buf = malloc(len + 1);
    memcpy(buf, data, len);
    buf[len] = '\0';
    
    char *line = strtok(buf, "\n");
    while (line) {
        if (strncmp(line, "tree ", 5) == 0) {
            bro_oid_from_string(&commit->tree_oid, line + 5);
        } else if (strncmp(line, "parent ", 7) == 0) {
            bro_oid_from_string(&commit->parent_oid, line + 7);
        } else if (strncmp(line, "author ", 7) == 0) {
            commit->author = strdup(line + 7);
        } else if (strncmp(line, "timestamp ", 10) == 0) {
            commit->timestamp = atol(line + 10);
        } else if (line[0] == '\0') {
            char *msg = strtok(NULL, "");
            if (msg) commit->message = strdup(msg);
            break;
        }
        line = strtok(NULL, "\n");
    }
    
    free(buf);
    return commit;
}

int bro_commit_write(bro_commit *commit, bro_oid *oid) {
    bro_object *obj = malloc(sizeof(bro_object));
    obj->type = BRO_OBJ_COMMIT;
    obj->data = bro_commit_serialize(commit, &obj->size);
    
    int ret = bro_object_write(obj, oid);
    
    free(obj->data);
    free(obj);
    return ret;
}

bro_commit *bro_commit_read(const bro_oid *oid) {
    bro_object *obj = bro_object_read(oid);
    if (!obj || obj->type != BRO_OBJ_COMMIT) {
        if (obj) { free(obj->data); free(obj); }
        return NULL;
    }
    
    bro_commit *commit = bro_commit_deserialize(obj->data, obj->size);
    free(obj->data);
    free(obj);
    return commit;
}
