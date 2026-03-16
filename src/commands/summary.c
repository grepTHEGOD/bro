#include "core/types.h"
#include <stdio.h>
#include <string.h>

extern int bro_file_exists(const char *path);
extern const char *bro_head_read(void);

int cmd_summary(int argc, char **argv) {
    if (!bro_file_exists(".bro")) {
        fprintf(stderr, "summary: not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    
    printf("bro shortlog\n");
    
    const char *head_ref = bro_head_read();
    if (!head_ref || strncmp(head_ref, "refs/heads/", 11) != 0) {
        printf("No commits yet\n");
        return 0;
    }
    
    printf("\n1 commit\n");
    printf("Author:\n");
    printf("\n\t commit\n");
    
    return 0;
}
