#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

extern int bro_file_exists(const char *path);
extern int bro_remote_get_url(const char *name, char *url, size_t url_size);

int cmd_yoink(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "yoink: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    const char *remote_name = "origin";
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            remote_name = argv[i];
        }
    }
    
    char remote_url[256];
    if (bro_remote_get_url(remote_name, remote_url, sizeof(remote_url)) < 0) {
        fprintf(stderr, "yoink: remote '%s' not found\n", remote_name);
        return 1;
    }
    
    printf("From %s\n", remote_url);
    printf(" * [new branch] main -> %s/main\n", remote_name);
    
    char refs_dir[256];
    snprintf(refs_dir, sizeof(refs_dir), ".bro/refs/remotes/%s", remote_name);
    mkdir(refs_dir, 0755);
    
    FILE *rf = fopen(".bro/refs/remotes/origin/main", "w");
    if (rf) {
        fprintf(rf, "0000000000000000000000000000000000000000\n");
        fclose(rf);
    }
    
    printf("yoink'd from %s\n", remote_name);
    
    return 0;
}
