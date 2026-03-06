#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int bro_remote_add(const char *name, const char *url) {
    FILE *f = fopen(".bro/config", "a");
    if (!f) return -1;
    
    fprintf(f, "[remote \"%s\"]\n", name);
    fprintf(f, "\turl = %s\n", url);
    fclose(f);
    
    return 0;
}

int bro_remote_remove(const char *name) {
    return 0;
}

bro_remote *bro_remote_get(const char *name) {
    FILE *f = fopen(".bro/config", "r");
    if (!f) return NULL;
    
    char line[512];
    char current_remote[64] = {0};
    char url[256] = {0};
    int in_remote = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "[remote \"", 9) == 0) {
            char *end = strstr(line, "\"]");
            if (end) {
                *end = '\0';
                strcpy(current_remote, line + 9);
                in_remote = 1;
                url[0] = '\0';
            }
        } else if (in_remote && strncmp(line, "\turl = ", 7) == 0) {
            line[strcspn(line, "\n")] = '\0';
            strcpy(url, line + 7);
            
            if (strcmp(current_remote, name) == 0) {
                fclose(f);
                bro_remote *remote = malloc(sizeof(bro_remote));
                remote->name = strdup(name);
                remote->url = strdup(url);
                return remote;
            }
        }
    }
    
    fclose(f);
    return NULL;
}

int bro_remote_get_url(const char *name, char *url, size_t url_size) {
    bro_remote *r = bro_remote_get(name);
    if (!r) return -1;
    
    strncpy(url, r->url, url_size - 1);
    url[url_size - 1] = '\0';
    free(r->name);
    free(r->url);
    free(r);
    return 0;
}

int bro_remote_fetch(const char *name) {
    bro_remote *remote = bro_remote_get(name);
    if (!remote) return -1;
    
    printf("Fetching from %s...\n", remote->url);
    
    free(remote->name);
    free(remote->url);
    free(remote);
    
    return 0;
}

int bro_remote_push(const char *name) {
    bro_remote *remote = bro_remote_get(name);
    if (!remote) return -1;
    
    printf("Pushing to %s...\n", remote->url);
    
    free(remote->name);
    free(remote->url);
    free(remote);
    
    return 0;
}

int bro_remote_list(bro_remote **remotes, size_t *count) {
    FILE *f = fopen(".bro/config", "r");
    if (!f) {
        *count = 0;
        *remotes = NULL;
        return 0;
    }
    
    char line[512];
    char current_remote[64] = {0};
    char url[256] = {0};
    int in_remote = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "[remote \"", 9) == 0) {
            char *end = strstr(line, "\"]");
            if (end) {
                *end = '\0';
                strcpy(current_remote, line + 9);
                in_remote = 1;
                url[0] = '\0';
            }
        } else if (in_remote && strncmp(line, "\turl = ", 7) == 0) {
            line[strcspn(line, "\n")] = '\0';
            strcpy(url, line + 7);
            
            bro_remote *r = malloc(sizeof(bro_remote));
            r->name = strdup(current_remote);
            r->url = strdup(url);
            
            *remotes = realloc(*remotes, (*count + 1) * sizeof(bro_remote));
            (*remotes)[*count] = *r;
            (*count)++;
            
            free(r);
        }
    }
    
    fclose(f);
    return 0;
}
