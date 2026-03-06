#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <time.h>
#include <dirent.h>

#define MAX_COMMITS 100
#define MAX_BRANCHES 20

typedef struct {
    char oid[64];
    char msg[128];
    char author[64];
    time_t time;
    int branch;
    int col;
} commit_info;

typedef struct {
    char name[64];
    char oid[64];
    int color;
} branch_info;

static branch_info branches[MAX_BRANCHES];
static int branch_count = 0;
static commit_info commits[MAX_COMMITS];
static int commit_count = 0;

int has_prefix(const char *str, const char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

void trim_newline(char *s) {
    s[strcspn(s, "\n")] = '\0';
}

void load_branches() {
    branch_count = 0;
    
    DIR *dir = opendir(".bro/refs/heads");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) && branch_count < MAX_BRANCHES) {
            if (entry->d_name[0] == '.') continue;
            strcpy(branches[branch_count].name, entry->d_name);
            
            char path[256];
            snprintf(path, sizeof(path), ".bro/refs/heads/%s", entry->d_name);
            FILE *f = fopen(path, "r");
            if (f) {
                fgets(branches[branch_count].oid, sizeof(branches[branch_count].oid), f);
                trim_newline(branches[branch_count].oid);
                fclose(f);
            }
            branches[branch_count].color = branch_count + 1;
            branch_count++;
        }
        closedir(dir);
    }
    
    DIR *remotedir = opendir(".bro/refs/remotes");
    if (remotedir) {
        struct dirent *entry;
        while ((entry = readdir(remotedir)) && branch_count < MAX_BRANCHES) {
            if (entry->d_name[0] == '.') continue;
            
            char remotepath[256];
            snprintf(remotepath, sizeof(remotepath), ".bro/refs/remotes/%s", entry->d_name);
            
            DIR *subdir = opendir(remotepath);
            if (subdir) {
                struct dirent *subentry;
                while ((subentry = readdir(subdir)) && branch_count < MAX_BRANCHES) {
                    if (subentry->d_name[0] == '.') continue;
                    
                    char reponame[128];
                    snprintf(reponame, sizeof(reponame), "%s/%s", entry->d_name, subentry->d_name);
                    
                    strcpy(branches[branch_count].name, reponame);
                    
                    char refpath[256];
                    snprintf(refpath, sizeof(refpath), ".bro/refs/remotes/%s", reponame);
                    
                    FILE *f = fopen(refpath, "r");
                    if (f) {
                        fgets(branches[branch_count].oid, sizeof(branches[branch_count].oid), f);
                        trim_newline(branches[branch_count].oid);
                        fclose(f);
                    }
                    branches[branch_count].color = branch_count + 1;
                    branch_count++;
                }
                closedir(subdir);
            }
        }
        closedir(remotedir);
    }
}

int find_branch_for_oid(const char *oid) {
    for (int i = 0; i < branch_count; i++) {
        if (strcmp(branches[i].oid, oid) == 0) {
            return i;
        }
    }
    return -1;
}

void load_commits() {
    commit_count = 0;
    
    char head_ref[64] = {0};
    FILE *f = fopen(".bro/HEAD", "r");
    if (f) {
        fgets(head_ref, sizeof(head_ref), f);
        trim_newline(head_ref);
        fclose(f);
    }
    
    char head_oid[64] = {0};
    if (has_prefix(head_ref, "ref: ")) {
        char ref_path[256];
        snprintf(ref_path, sizeof(ref_path), ".bro/%s", head_ref + 5);
        f = fopen(ref_path, "r");
        if (f) {
            fgets(head_oid, sizeof(head_oid), f);
            trim_newline(head_oid);
            fclose(f);
        }
    } else if (head_ref[0] != '\0') {
        strcpy(head_oid, head_ref);
    }
    
    for (int i = 0; i < branch_count && commit_count < MAX_COMMITS; i++) {
        if (branches[i].oid[0] == '\0') continue;
        
        strcpy(commits[commit_count].oid, branches[i].oid);
        snprintf(commits[commit_count].msg, sizeof(commits[commit_count].msg), 
                 "Commit from %s", branches[i].name);
        snprintf(commits[commit_count].author, sizeof(commits[commit_count].author), 
                 "bro_user");
        commits[commit_count].time = time(NULL) - (commit_count * 3600);
        commits[commit_count].branch = i;
        commits[commit_count].col = i % 6;
        commit_count++;
    }
    
    if (head_oid[0] != '\0' && find_branch_for_oid(head_oid) < 0) {
        if (commit_count < MAX_COMMITS) {
            strcpy(commits[commit_count].oid, head_oid);
            strcpy(commits[commit_count].msg, "HEAD commit");
            strcpy(commits[commit_count].author, "bro_user");
            commits[commit_count].time = time(NULL);
            commits[commit_count].branch = -1;
            commits[commit_count].col = 7;
            commit_count++;
        }
    }
}

void draw_graph(WINDOW *win, int start_row, int start_col) {
    const char *colors[] = {"│", "│", "│", "│", "│", "│"};
    const char *horiz = "─";
    const char *top_left = "┌";
    const char *top_right = "┐";
    const char *mid_left = "├";
    const char *bottom_left = "└";
    
    for (int i = 0; i < commit_count && i < 20; i++) {
        int row = start_row + i;
        int col = start_col;
        
        if (i == 0) {
            wattron(win, COLOR_PAIR(commits[i].col + 1));
            mvwprintw(win, row, col, "%s%s", top_left, horiz);
            col += 2;
        } else if (i == commit_count - 1) {
            wattron(win, COLOR_PAIR(commits[i].col + 1));
            mvwprintw(win, row, col, "%s%s", bottom_left, horiz);
            col += 2;
        } else {
            wattron(win, COLOR_PAIR(commits[i].col + 1));
            mvwprintw(win, row, col, "%s%s", mid_left, horiz);
            col += 2;
        }
        
        mvwprintw(win, row, col, "● ");
        
        wattron(win, COLOR_PAIR(8));
        mvwprintw(win, row, col + 2, "%.7s ", commits[i].oid);
        
        wattron(win, COLOR_PAIR(1));
        mvwprintw(win, row, col + 12, "%s", commits[i].msg);
        
        if (commits[i].branch >= 0) {
            wattron(win, COLOR_PAIR(commits[i].branch + 1));
            mvwprintw(win, row, col + 44, " %s", branches[commits[i].branch].name);
        }
        
        wattroff(win, COLOR_PAIR(1) | COLOR_PAIR(8));
    }
}

void draw_sidebar(WINDOW *win, int height) {
    int y = 2;
    
    wattron(win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(win, y, 2, "🏴 SQUADS");
    wattroff(win, A_BOLD);
    
    for (int i = 0; i < branch_count && y < height - 5; i++) {
        y += 2;
        wattron(win, COLOR_PAIR(i + 1));
        if (i < 9) {
            mvwprintw(win, y, 2, "%d. %s", i + 1, branches[i].name);
        } else {
            mvwprintw(win, y, 2, "   %s", branches[i].name);
        }
        wattroff(win, COLOR_PAIR(i + 1));
    }
    
    y += 3;
    wattron(win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(win, y, 2, "⌨️  COMMANDS");
    wattroff(win, A_BOLD);
    y += 2;
    mvwprintw(win, y, 2, "q - quit");
    y += 1;
    mvwprintw(win, y, 2, "r - refresh");
    y += 1;
    mvwprintw(win, y, 2, "l - launch (push)");
}

int cmd_tui(int argc, char **argv) {
    initscr();
    start_color();
    curs_set(0);
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_RED, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_CYAN, COLOR_BLACK);
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    WINDOW *sidebar = newwin(height - 2, 20, 1, 1);
    WINDOW *graph = newwin(height - 2, width - 24, 1, 21);
    WINDOW *title = newwin(1, width, 0, 0);
    
    wbkgd(title, COLOR_PAIR(3));
    wattron(title, A_BOLD);
    mvwprintw(title, 0, 2, "🏁 BRO GRAPH - Visualizing your vibes");
    wattroff(title, A_BOLD);
    
    box(sidebar, 0, 0);
    box(graph, 0, 0);
    
    load_branches();
    load_commits();
    
    draw_sidebar(sidebar, height - 2);
    draw_graph(graph, 2, 2);
    
    wrefresh(title);
    wrefresh(sidebar);
    wrefresh(graph);
    
    int ch;
    while ((ch = getch()) != 'q') {
        if (ch == 'r') {
            load_branches();
            load_commits();
            werase(sidebar);
            werase(graph);
            box(sidebar, 0, 0);
            box(graph, 0, 0);
            draw_sidebar(sidebar, height - 2);
            draw_graph(graph, 2, 2);
            wrefresh(sidebar);
            wrefresh(graph);
        }
    }
    
    delwin(sidebar);
    delwin(graph);
    delwin(title);
    endwin();
    
    return 0;
}
