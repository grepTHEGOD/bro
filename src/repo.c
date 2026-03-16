#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

static bro_repository *current_repo = NULL;

bro_repository *bro_repo_create(const char *path) {
    bro_repository *repo = calloc(1, sizeof(bro_repository));
    repo->path = strdup(path);
    return repo;
}

void bro_repo_free(bro_repository *repo) {
    if (!repo) return;
    free(repo->path);
    free(repo);
}

bro_repository *bro_repo_open(const char *path) {
    char bro_path[512];
    snprintf(bro_path, sizeof(bro_path), "%s/.bro", path);
    
    struct stat st;
    if (stat(bro_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return NULL;
    }
    
    return bro_repo_create(path);
}

int bro_repo_is_valid(const char *path) {
    char bro_path[512];
    snprintf(bro_path, sizeof(bro_path), "%s/.bro", path);
    
    struct stat st;
    return (stat(bro_path, &st) == 0 && S_ISDIR(st.st_mode));
}

const char *bro_repo_get_path(bro_repository *repo) {
    return repo ? repo->path : NULL;
}

int bro_repo_set_current(bro_repository *repo) {
    current_repo = repo;
    return 0;
}

bro_repository *bro_repo_get_current(void) {
    return current_repo;
}
