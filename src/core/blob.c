#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bro_object *bro_object_read(const bro_oid *oid);
int bro_object_write(bro_object *obj, bro_oid *oid);

bro_blob *bro_blob_create(const char *data, size_t len) {
    bro_blob *blob = malloc(sizeof(bro_blob));
    blob->data = malloc(len + 1);
    memcpy(blob->data, data, len);
    blob->data[len] = '\0';
    blob->len = len;
    return blob;
}

void bro_blob_free(bro_blob *blob) {
    if (blob) {
        free(blob->data);
        free(blob);
    }
}

bro_object *bro_blob_to_object(bro_blob *blob) {
    bro_object *obj = malloc(sizeof(bro_object));
    obj->type = BRO_OBJ_BLOB;
    obj->data = blob->data;
    obj->size = blob->len;
    return obj;
}

bro_blob *bro_blob_from_object(bro_object *obj) {
    if (obj->type != BRO_OBJ_BLOB) return NULL;
    bro_blob *blob = malloc(sizeof(bro_blob));
    blob->data = malloc(obj->size + 1);
    memcpy(blob->data, obj->data, obj->size);
    blob->data[obj->size] = '\0';
    blob->len = obj->size;
    return blob;
}

int bro_blob_write(bro_blob *blob, bro_oid *oid) {
    bro_object *obj = bro_blob_to_object(blob);
    int ret = bro_object_write(obj, oid);
    free(obj);
    return ret;
}

bro_blob *bro_blob_read(const bro_oid *oid) {
    bro_object *obj = bro_object_read(oid);
    if (!obj) return NULL;
    bro_blob *blob = bro_blob_from_object(obj);
    free(obj->data);
    free(obj);
    return blob;
}
