#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

extern int bro_file_exists(const char *path);

int cmd_scrub(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "scrub: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    int force = 0;
    int dry_run = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--dry-run") == 0) {
            dry_run = 1;
        }
    }
    
    if (!force && !dry_run) {
        fprintf(stderr, "scrub: use -f to force removal of untracked files\n");
        return 1;
    }
    
    int count = 0;
    
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (strcmp(entry->d_name, ".bro") == 0) continue;
            
            struct stat st;
            if (stat(entry->d_name, &st) != 0) continue;
            if (S_ISDIR(st.st_mode)) continue;
            
            if (dry_run) {
                printf("Would remove: %s\n", entry->d_name);
            } else {
                if (unlink(entry->d_name) == 0) {
                    printf("Removed: %s\n", entry->d_name);
                }
            }
            count++;
        }
        closedir(dir);
    }
    
    if (count == 0) {
        printf("No files to remove\n");
    } else if (dry_run) {
        printf("Would remove %d file(s)\n", count);
    } else {
        printf("Removed %d file(s)\n", count);
    }
    
    return 0;
}
