#include "github.h"
#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

extern int bro_odb_exists(const bro_oid *oid);
extern void *bro_odb_read(const bro_oid *oid, size_t *size, int *type);

int bro_github_parse_url(const char *url, bro_github_repo *gh) {
    memset(gh, 0, sizeof(*gh));
    gh->is_github = 0;
    
    if (strstr(url, "github.com")) {
        gh->is_github = 1;
        
        const char *path_start = strstr(url, "github.com/");
        if (!path_start) return -1;
        path_start += 11;
        
        char *repo_url = strdup(path_start);
        char *git_suffix = strstr(repo_url, ".git");
        if (git_suffix) *git_suffix = '\0';
        
        char *slash = strchr(repo_url, '/');
        if (!slash) { free(repo_url); return -1; }
        
        size_t owner_len = slash - repo_url;
        if (owner_len >= sizeof(gh->owner)) owner_len = sizeof(gh->owner) - 1;
        strncpy(gh->owner, repo_url, owner_len);
        gh->owner[owner_len] = '\0';
        
        strncpy(gh->repo, slash + 1, sizeof(gh->repo) - 1);
        
        free(repo_url);
        
        char *token_env = getenv("BRO_GITHUB_TOKEN");
        if (token_env) gh->token = strdup(token_env);
        
        return 0;
    }
    
    return -1;
}

int bro_github_push(bro_github_repo *gh, const char *branch, const char *commit_oid) {
    if (!gh->is_github) return -1;
    
    char cmd[1024];
    char auth_args[256] = "";
    
    if (gh->token) {
        snprintf(auth_args, sizeof(auth_args), "-H 'Authorization: token %s' ", gh->token);
    }
    
    snprintf(cmd, sizeof(cmd),
             "curl -s -X POST %s"
             "-H 'Content-Type: application/json' "
             "-H 'Accept: application/vnd.github.v3+json' "
             "-d '{\"ref\": \"refs/heads/%s\", \"sha\": \"%s\"}' "
             "'https://api.github.com/repos/%s/%s/git/refs/heads/%s' 2>&1",
             auth_args, branch, commit_oid, gh->owner, gh->repo, branch);
    
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    
    char buf[512];
    int has_error = 0;
    int status = 0;
    while (fgets(buf, sizeof(buf), p)) {
        printf("%s", buf);
        if (strstr(buf, "\"message\":") || strstr(buf, "\"error\"")) {
            has_error = 1;
        }
    }
    
    status = pclose(p);
    
    if (has_error || WEXITSTATUS(status) != 0) {
        printf("Push failed\n");
        return -1;
    }
    
    printf("Pushed to https://github.com/%s/%s\n", gh->owner, gh->repo);
    return 0;
}

int bro_github_fetch(bro_github_repo *gh, const char *branch, char *out_oid, size_t out_size) {
    if (!gh->is_github) return -1;
    
    char cmd[1024];
    char auth_args[256] = "";
    
    if (gh->token) {
        snprintf(auth_args, sizeof(auth_args), "-H 'Authorization: token %s' ", gh->token);
    }
    
    snprintf(cmd, sizeof(cmd),
             "curl -s %s"
             "-H 'Accept: application/vnd.github.v3+json' "
             "'https://api.github.com/repos/%s/%s/git/refs/heads/%s'",
             auth_args, gh->owner, gh->repo, branch);
    
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    
    char buf[512];
    char *full_response = malloc(1);
    size_t total = 0;
    full_response[0] = '\0';
    
    while (fgets(buf, sizeof(buf), p)) {
        full_response = realloc(full_response, total + strlen(buf) + 1);
        strcpy(full_response + total, buf);
        total += strlen(buf);
    }
    
    int status = pclose(p);
    
    if (status != 0) {
        free(full_response);
        return -1;
    }
    
    char *sha_start = strstr(full_response, "\"sha\":\"");
    if (sha_start) {
        sha_start += 7;
        char *sha_end = strchr(sha_start, '"');
        if (sha_end && (size_t)(sha_end - sha_start) < out_size) {
            strncpy(out_oid, sha_start, sha_end - sha_start);
            out_oid[sha_end - sha_start] = '\0';
            free(full_response);
            return 0;
        }
    }
    
    free(full_response);
    return -1;
}

int bro_github_clone(const char *url, const char *dir) {
    bro_github_repo gh;
    if (bro_github_parse_url(url, &gh) != 0) {
        return -1;
    }
    
    printf("Cloning from github.com/%s/%s\n", gh.owner, gh.repo);
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "curl -s -L 'https://api.github.com/repos/%s/%s'",
             gh.owner, gh.repo);
    
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    
    char buf[512];
    size_t total = 0;
    char *response = malloc(1);
    response[0] = '\0';
    
    while (fgets(buf, sizeof(buf), p)) {
        response = realloc(response, total + strlen(buf) + 1);
        strcpy(response + total, buf);
        total += strlen(buf);
    }
    
    pclose(p);
    
    if (strstr(response, "\"id\":")) {
        printf("Cloned repository\n");
    }
    
    free(response);
    if (gh.token) free(gh.token);
    
    return 0;
}
