#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

int cmd_init(int argc, char **argv) {
    struct stat st;
    int error = stat(".bro", &st);
    if (error == 0) {
        if (S_ISDIR(st.st_mode)) {
            printf("Folder already exists, bro");
            return 1;
        }
    }
    // Create .bro directory
    if (mkdir(".bro", 0755) != 0 && errno != EEXIST) {
        perror("Failed to create .bro");
        return 1;
    }
    
    // Create .bro/objects
    if (mkdir(".bro/objects", 0755) != 0 && errno != EEXIST) {
        perror("Failed to create .bro/objects");
        return 1;
    }
    
    // Create .bro/refs
    if (mkdir(".bro/refs", 0755) != 0 && errno != EEXIST) {
        perror("Failed to create .bro/refs");
        return 1;
    }
    
    // Create .bro/refs/heads
    if (mkdir(".bro/refs/heads", 0755) != 0 && errno != EEXIST) {
        perror("Failed to create .bro/refs/heads");
        return 1;
    }
    
    // Create .bro/HEAD file pointing to main branch
    FILE *head = fopen(".bro/HEAD", "w");
    if (!head) {
        perror("Failed to create .bro/HEAD");
        return 1;
    }
    fprintf(head, "ref: refs/heads/main\n");
    fclose(head);
    
    printf("Initialized empty Bro repository in .bro/ 😎\n");
    return 0;
}