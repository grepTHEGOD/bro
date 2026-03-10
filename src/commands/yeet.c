#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int cmd_yeet(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bro yeet -m <message>\n");
        return 1;
    }

    const char *message = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            message = argv[i + 1];
            break;
        }
    }

    if (!message) {
        fprintf(stderr, "Error: Commit message is required. Use -m <message>.\n");
        return 1;
    }

    FILE *commit_file = fopen(".bro/HEAD", "r");
    if (!commit_file) {
        fprintf(stderr, "Error: Not a bro repository. Run 'bro init' first.\n");
        return 1;
    }
    fclose(commit_file);

    char commit_path[256];
    snprintf(commit_path, sizeof(commit_path), ".bro/objects/commit-%ld", time(NULL));

    FILE *commit = fopen(commit_path, "w");
    if (!commit) {
        fprintf(stderr, "Error: Could not create commit object.\n");
        return 1;
    }

    fprintf(commit, "Commit: %s\n", message);
    fclose(commit);

    printf("[bro yeet] %s\n", message);
    return 0;
}
