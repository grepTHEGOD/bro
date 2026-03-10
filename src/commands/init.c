#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int cmd_init(int argc, char **argv) {
    const char *path = ".";
    if (argc > 1) {
        path = argv[1];
    }
    
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", path);
    mkdir(dir, 0755);
    
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
    
    snprintf(dir, sizeof(dir), "%s/.bro/info", path);
    mkdir(dir, 0755);
    
    FILE *f;
    snprintf(dir, sizeof(dir), "%s/.bro/HEAD", path);
    f = fopen(dir, "w");
    fprintf(f, "ref: refs/heads/main\n");
    fclose(f);
    
    snprintf(dir, sizeof(dir), "%s/.bro/config", path);
    f = fopen(dir, "w");
    fprintf(f, "[core]\n\trepositoryformatversion = 0\n\tfilemode = true\n\tbare = false\n");
    fclose(f);

    printf("Initialized empty bro repository in %s/.bro/\n", path);
    return 0;
}
