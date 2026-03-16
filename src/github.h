#ifndef BRO_GITHUB_H
#define BRO_GITHUB_H

#include <stddef.h>

typedef struct {
    char owner[128];
    char repo[128];
    char *token;
    int is_github;
} bro_github_repo;

int bro_github_parse_url(const char *url, bro_github_repo *gh);
int bro_github_push(bro_github_repo *gh, const char *branch, const char *commit_oid);
int bro_github_fetch(bro_github_repo *gh, const char *branch, char *out_oid, size_t out_size);
int bro_github_clone(const char *url, const char *dir);

#endif
