#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int bro_file_exists(const char *path);

int cmd_config_vibe(int argc, char **argv) {
    int global = 0;
    char *key = NULL;
    char *value = NULL;
    int list_all = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--global") == 0) {
            global = 1;
        } else if (strcmp(argv[i], "--list") == 0 || strcmp(argv[i], "-l") == 0) {
            list_all = 1;
        } else if (argv[i][0] == '-' ) {
            continue;
        } else if (!key) {
            key = argv[i];
        } else if (!value) {
            value = argv[i];
        }
    }
    
    const char *config_file = global ? "~/.broconfig" : ".bro/config";
    
    if (list_all) {
        FILE *f = fopen(".bro/config", "r");
        if (f) {
            char line[512];
            while (fgets(line, sizeof(line), f)) {
                printf("%s", line);
            }
            fclose(f);
        }
        return 0;
    }
    
    if (!key) {
        fprintf(stderr, "Usage: bro config-vibe [--global] <key> [value]\n");
        return 1;
    }
    
    if (value) {
        FILE *f = fopen(".bro/config", "a");
        if (f) {
            char *section = strchr(key, '.');
            if (section) {
                char * subsection = strchr(section + 1, '.');
                if (subsection) {
                    *section = '\0';
                    *subsection = '\0';
                    fprintf(f, "[%s \"%s\"]\n", key, section + 1);
                    *section = '.';
                    *subsection = '.';
                    fprintf(f, "\t%s = %s\n", subsection + 1, value);
                } else {
                    *section = '\0';
                    fprintf(f, "[%s \"%s\"]\n", key, section + 1);
                    *section = '.';
                    fprintf(f, "\turl = %s\n", value);
                }
            } else {
                fprintf(f, "\t%s = %s\n", key, value);
            }
            fclose(f);
        }
        printf("Set %s = %s\n", key, value);
    } else {
        FILE *f = fopen(".bro/config", "r");
        if (f) {
            char line[512];
            char current_section[64] = "";
            while (fgets(line, sizeof(line), f)) {
                if (line[0] == '[') {
                    char *end = strstr(line, "]");
                    if (end) {
                        *end = '\0';
                        strcpy(current_section, line + 1);
                    }
                } else if (strncmp(line, key, strlen(key)) == 0) {
                    char *eq = strchr(line, '=');
                    if (eq) {
                        printf("%s\n", eq + 1);
                    }
                }
            }
            fclose(f);
        }
    }
    
    return 0;
}
