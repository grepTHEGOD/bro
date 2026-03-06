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
    free(repo->worktree);
    for (size_t i = 0; i < repo->remote_count; i++) {
        free(repo->remotes[i].name);
        free(repo->remotes[i].url);
    }
    free(repo->remotes);
    free(repo);
}

int bro_repo_init(const char *path) {
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/.bro", path);
    mkdir(dir, 0755);
    
    snprintf(dir, sizeof(dir), "%s/.bro/objects", path);
    mkdir(dir, 0755);
    
    snprintf(dir, sizeof(dir), "%s/.bro/refs", path);
    mkdir(dir, 0755);
    
    snprintf(dir, sizeof(dir), "%s/.bro/refs/heads", path);
    mkdir(dir, 0755);
    
    snprintf(dir, sizeof(dir), "%s/.bro/refs/tags", path);
    mkdir(dir, 0755);
    
    FILE *f = fopen(".bro/HEAD", "w");
    fprintf(f, "ref: refs/heads/main\n");
    fclose(f);
    
    return 0;
}

bro_repository *bro_repo_open(const char *path) {
    char head_path[512];
    snprintf(head_path, sizeof(head_path), "%s/.bro/HEAD", path);
    
    FILE *f = fopen(head_path, "r");
    if (!f) return NULL;
    
    char ref[256];
    fgets(ref, sizeof(ref), f);
    fclose(f);
    
    if (strncmp(ref, "ref: ", 5) == 0) {
        char *ref_path = ref + 5;
        ref_path[strcspn(ref_path, "\n")] = '\0';
        
        char full_ref[512];
        snprintf(full_ref, sizeof(full_ref), "%s/.bro/%s", path, ref_path);
        
        f = fopen(full_ref, "r");
        if (!f) return NULL;
        
        char oid_str[64];
        fgets(oid_str, sizeof(oid_str), f);
        fclose(f);
        
        oid_str[strcspn(oid_str, "\n")] = '\0';
    }
    
    bro_repository *repo = bro_repo_create(path);
    return repo;
}

bro_repository *bro_repo_get(void) {
    if (current_repo) return current_repo;
    
    char cwd[512];
    if (!getcwd(cwd, sizeof(cwd))) return NULL;
    
    while (strcmp(cwd, "/") != 0) {
        char bro_path[512];
        snprintf(bro_path, sizeof(bro_path), "%s/.bro", cwd);
        
        struct stat st;
        if (stat(bro_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            current_repo = bro_repo_open(cwd);
            return current_repo;
        }
        
        char parent[512];
        snprintf(parent, sizeof(parent), "%s/..", cwd);
        realpath(parent, cwd);
    }
    
    return NULL;
}

const char *bro_repo_path(bro_repository *repo) {
    return repo->path;
}

int bro_repo_config_get(bro_repository *repo, const char *key, char **value) {
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/.bro/config", repo->path);
    
    FILE *f = fopen(config_path, "r");
    if (!f) return -1;
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            if (strcmp(line, key) == 0) {
                *value = strdup(eq + 1);
                (*value)[strcspn(*value, "\n")] = '\0';
                fclose(f);
                return 0;
            }
        }
    }
    
    fclose(f);
    return -1;
}

int bro_repo_config_set(bro_repository *repo, const char *key, const char *value) {
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/.bro/config", repo->path);
    
    FILE *f = fopen(config_path, "a");
    if (!f) return -1;
    
    fprintf(f, "%s=%s\n", key, value);
    fclose(f);
    
    return 0;
}
