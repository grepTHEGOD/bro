#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

extern int bro_file_exists(const char *path);
extern int bro_mkdir(const char *path);

int cmd_steal(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro steal <url> [directory]\n");
        return 1;
    }
    
    const char *url = argv[1];
    const char *dir = ".";
    
    if (argc > 2) {
        dir = argv[2];
    }
    
    if (bro_file_exists(dir)) {
        fprintf(stderr, "steal: directory '%s' already exists\n", dir);
        return 1;
    }
    
    bro_mkdir(dir);
    
    char cwd[512];
    getcwd(cwd, sizeof(cwd));
    
    chdir(dir);
    
    bro_mkdir(".bro");
    bro_mkdir(".bro/objects");
    bro_mkdir(".bro/refs");
    bro_mkdir(".bro/refs/heads");
    bro_mkdir(".bro/refs/tags");
    bro_mkdir(".bro/info");
    
    FILE *f = fopen(".bro/HEAD", "w");
    if (f) {
        fprintf(f, "ref: refs/heads/main\n");
        fclose(f);
    }
    
    f = fopen(".bro/config", "w");
    if (f) {
        fprintf(f, "[core]\n\trepositoryformatversion = 0\n\tfilemode = true\n\tbare = false\n");
        fprintf(f, "[remote \"origin\"]\n\turl = %s\n\tfetch = +refs/heads/*:refs/remotes/origin/*\n", url);
        fclose(f);
    }
    
    chdir(cwd);
    
    printf("Cloning from %s\n", url);
    printf("Initialized empty bro repository in %s/.bro/\n", dir);
    printf("warning: You appear to have cloned an empty repository.\n");
    
    return 0;
}
