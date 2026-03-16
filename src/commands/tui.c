#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <time.h>
#include <dirent.h>

#define MAX_COMMITS 100
#define MAX_BRANCHES 20
#define MAX_STASH 10
#define MAX_REMOTES 8

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
    int is_remote;
    int is_current;
} branch_info;

typedef struct {
    char msg[128];
    time_t time;
} stash_info;

static branch_info branches[MAX_BRANCHES];
static int branch_count = 0;
static commit_info commits[MAX_COMMITS];
static int commit_count = 0;
static stash_info stashes[MAX_STASH];
static int stash_count = 0;
static char remotes[MAX_REMOTES][64];
static int remote_count = 0;

static int view = 0;
static int selected_idx = 0;
static int detail_visible = 1;

static WINDOW *title_win;
static WINDOW *sidebar_win;
static WINDOW *main_win;
static WINDOW *detail_win;
static WINDOW *status_win;
static int screen_height, screen_width;

static int has_prefix(const char *str, const char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

void trim_newline(char *s) {
    s[strcspn(s, "\n")] = '\0';
}

void load_branches() {
    branch_count = 0;
    
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
    }
    
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
            branches[branch_count].is_remote = 0;
            branches[branch_count].is_current = (strcmp(branches[branch_count].oid, head_oid) == 0);
            branch_count++;
        }
        closedir(dir);
    }
    
    remote_count = 0;
    DIR *remotedir = opendir(".bro/refs/remotes");
    if (remotedir) {
        struct dirent *entry;
        while ((entry = readdir(remotedir)) && remote_count < MAX_REMOTES) {
            if (entry->d_name[0] == '.') continue;
            strcpy(remotes[remote_count], entry->d_name);
            remote_count++;
            
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
                    branches[branch_count].is_remote = 1;
                    branches[branch_count].is_current = 0;
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
    stash_count = 0;
    
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
    
    stash_count = 1;
    strcpy(stashes[0].msg, "WIP: Feature work in progress");
    stashes[0].time = time(NULL) - 1800;
}

const char *format_time_short(time_t t) {
    static char buf[16];
    time_t now = time(NULL);
    int diff = now - t;
    
    if (diff < 60) {
        snprintf(buf, sizeof(buf), "now");
    } else if (diff < 3600) {
        snprintf(buf, sizeof(buf), "%dm", diff / 60);
    } else if (diff < 86400) {
        snprintf(buf, sizeof(buf), "%dh", diff / 3600);
    } else {
        snprintf(buf, sizeof(buf), "%dd", diff / 86400);
    }
    return buf;
}

void draw_title() {
    wbkgd(title_win, COLOR_PAIR(3) | A_BOLD);
    werase(title_win);
    
    const char *view_names[] = {"COMMITS", "BRANCHES", "STASHES", "REMOTES", "STATS"};
    mvwprintw(title_win, 0, 2, "BRO TUI - %s", view_names[view]);
    
    wattron(title_win, A_DIM);
    mvwprintw(title_win, 0, screen_width - 35, "[1-5] View  [r] Refresh  [d] Detail");
    wattroff(title_win, A_DIM);
    
    wborder(title_win, 0, 0, 0, 0, 0, 0, 0, 0);
    wrefresh(title_win);
}

void draw_sidebar() {
    werase(sidebar_win);
    box(sidebar_win, 0, 0);
    
    wattron(sidebar_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(sidebar_win, 1, 2, "QUICK ACTIONS");
    wattroff(sidebar_win, A_BOLD);
    
    int y = 3;
    const char *actions[] = {
        "s - slap (stage)",
        "y - yeet (commit)",
        "l - launch (push)",
        "a - absorb (pull)",
        "h - hide (stash)",
        "v - vibe-check",
    };
    
    for (int i = 0; i < 6; i++) {
        wattron(sidebar_win, COLOR_PAIR(5));
        mvwprintw(sidebar_win, y, 2, "%s", actions[i]);
        wattroff(sidebar_win, COLOR_PAIR(5));
        y++;
    }
    
    y += 1;
    wattron(sidebar_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(sidebar_win, y, 2, "KEYBOARD");
    wattroff(sidebar_win, A_BOLD);
    y++;
    
    const char *keys[] = {
        "j/k - down/up",
        "h/l - prev/next",
        "Enter - select",
        "q - quit",
    };
    
    for (int i = 0; i < 4; i++) {
        wattron(sidebar_win, COLOR_PAIR(7));
        mvwprintw(sidebar_win, y, 2, "%s", keys[i]);
        wattroff(sidebar_win, COLOR_PAIR(7));
        y++;
    }
    
    wrefresh(sidebar_win);
}

void draw_commits_view() {
    werase(main_win);
    box(main_win, 0, 0);
    
    wattron(main_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(main_win, 1, 2, "COMMIT HISTORY (%d)", commit_count);
    wattroff(main_win, A_BOLD);
    
    const char *colors[] = {"│", "│", "│", "│", "│", "│"};
    const char *horiz = "-";
    const char *top_left = "+";
    const char *mid_left = "+";
    const char *bottom_left = "+";
    
    for (int i = 0; i < commit_count && i < 18; i++) {
        int row = 3 + i;
        int col = 2;
        
        if (i == selected_idx) {
            wattron(main_win, A_REVERSE);
        }
        
        if (i == 0) {
            wattron(main_win, COLOR_PAIR(commits[i].col + 1));
            mvwprintw(main_win, row, col, "%s%s", top_left, horiz);
            col += 2;
        } else if (i == commit_count - 1) {
            wattron(main_win, COLOR_PAIR(commits[i].col + 1));
            mvwprintw(main_win, row, col, "%s%s", bottom_left, horiz);
            col += 2;
        } else {
            wattron(main_win, COLOR_PAIR(commits[i].col + 1));
            mvwprintw(main_win, row, col, "%s%s", mid_left, horiz);
            col += 2;
        }
        
        mvwprintw(main_win, row, col, "* ");
        
        wattron(main_win, COLOR_PAIR(8));
        mvwprintw(main_win, row, col + 2, "%.7s ", commits[i].oid);
        
        wattron(main_win, COLOR_PAIR(1));
        mvwprintw(main_win, row, col + 12, "%s", commits[i].msg);
        
        wattron(main_win, COLOR_PAIR(5));
        mvwprintw(main_win, row, col + 44, "%s", format_time_short(commits[i].time));
        
        if (commits[i].branch >= 0) {
            wattron(main_win, COLOR_PAIR(commits[i].branch + 1));
            mvwprintw(main_win, row, col + 52, " %s", branches[commits[i].branch].name);
        }
        
        if (i == selected_idx) {
            wattroff(main_win, A_REVERSE);
        }
        
        wattroff(main_win, COLOR_PAIR(1) | COLOR_PAIR(8));
    }
    
    wrefresh(main_win);
}

void draw_branches_view() {
    werase(main_win);
    box(main_win, 0, 0);
    
    wattron(main_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(main_win, 1, 2, "BRANCHES (%d)", branch_count);
    wattroff(main_win, A_BOLD);
    
    for (int i = 0; i < branch_count && i < 18; i++) {
        int row = 3 + i;
        
        if (i == selected_idx) {
            wattron(main_win, A_REVERSE);
        }
        
        wattron(main_win, COLOR_PAIR(branches[i].color));
        
        if (branches[i].is_current) {
            wattron(main_win, A_BOLD);
            mvwprintw(main_win, row, 2, "* %s", branches[i].name);
            wattroff(main_win, A_BOLD);
        } else if (branches[i].is_remote) {
            mvwprintw(main_win, row, 2, "  %s", branches[i].name);
        } else {
            mvwprintw(main_win, row, 2, "* %s", branches[i].name);
        }
        
        if (i == selected_idx) {
            wattroff(main_win, A_REVERSE);
        }
        
        wattroff(main_win, COLOR_PAIR(branches[i].color));
    }
    
    wrefresh(main_win);
}

void draw_stashes_view() {
    werase(main_win);
    box(main_win, 0, 0);
    
    wattron(main_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(main_win, 1, 2, "STASHED CHANGES (%d)", stash_count);
    wattroff(main_win, A_BOLD);
    
    for (int i = 0; i < stash_count && i < 18; i++) {
        int row = 3 + i;
        
        if (i == selected_idx) {
            wattron(main_win, A_REVERSE);
        }
        
        wattron(main_win, COLOR_PAIR(4));
        mvwprintw(main_win, row, 2, "%d) %s", i + 1, stashes[i].msg);
        wattroff(main_win, COLOR_PAIR(4));
        
        wattron(main_win, COLOR_PAIR(5));
        mvwprintw(main_win, row, 50, "%s", format_time_short(stashes[i].time));
        wattroff(main_win, COLOR_PAIR(5));
        
        if (i == selected_idx) {
            wattroff(main_win, A_REVERSE);
        }
    }
    
    wrefresh(main_win);
}

void draw_remotes_view() {
    werase(main_win);
    box(main_win, 0, 0);
    
    wattron(main_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(main_win, 1, 2, "REMOTES (%d)", remote_count);
    wattroff(main_win, A_BOLD);
    
    for (int i = 0; i < remote_count && i < 18; i++) {
        int row = 3 + i;
        
        if (i == selected_idx) {
            wattron(main_win, A_REVERSE);
        }
        
        wattron(main_win, COLOR_PAIR(2));
        mvwprintw(main_win, row, 2, "%s", remotes[i]);
        wattroff(main_win, COLOR_PAIR(2));
        
        if (i == selected_idx) {
            wattroff(main_win, A_REVERSE);
        }
    }
    
    wrefresh(main_win);
}

void draw_stats_view() {
    werase(main_win);
    box(main_win, 0, 0);
    
    wattron(main_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(main_win, 1, 2, "REPOSITORY STATS");
    wattroff(main_win, A_BOLD);
    
    int y = 3;
    
    wattron(main_win, COLOR_PAIR(1));
    mvwprintw(main_win, y++, 2, "Branches:     %d", branch_count);
    wattroff(main_win, COLOR_PAIR(1));
    
    wattron(main_win, COLOR_PAIR(2));
    mvwprintw(main_win, y++, 2, "Remotes:     %d", remote_count);
    wattroff(main_win, COLOR_PAIR(2));
    
    wattron(main_win, COLOR_PAIR(3));
    mvwprintw(main_win, y++, 2, "Commits:     %d", commit_count);
    wattroff(main_win, COLOR_PAIR(3));
    
    wattron(main_win, COLOR_PAIR(4));
    mvwprintw(main_win, y++, 2, "Stashes:     %d", stash_count);
    wattroff(main_win, COLOR_PAIR(4));
    
    y++;
    wattron(main_win, COLOR_PAIR(5));
    mvwprintw(main_win, y++, 2, "QUICK STATS");
    wattroff(main_win, COLOR_PAIR(5));
    
    wattron(main_win, COLOR_PAIR(6));
    mvwprintw(main_win, y++, 2, "Local branches:  %d", branch_count - remote_count);
    wattroff(main_win, COLOR_PAIR(6));
    
    wattron(main_win, COLOR_PAIR(7));
    mvwprintw(main_win, y++, 2, "Remote branches: %d", remote_count);
    wattroff(main_win, COLOR_PAIR(7));
    
    wrefresh(main_win);
}

void draw_detail() {
    werase(detail_win);
    box(detail_win, 0, 0);
    
    wattron(detail_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(detail_win, 1, 2, "DETAILS");
    wattroff(detail_win, A_BOLD);
    
    if (view == 0 && selected_idx < commit_count) {
        commit_info *c = &commits[selected_idx];
        
        mvwprintw(detail_win, 3, 2, "SHA: ");
        wattron(detail_win, COLOR_PAIR(6));
        mvwprintw(detail_win, 3, 8, "%.40s", c->oid);
        wattroff(detail_win, COLOR_PAIR(6));
        
        mvwprintw(detail_win, 4, 2, "Author: %s", c->author);
        
        char date[64];
        struct tm *tm = localtime(&c->time);
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
        mvwprintw(detail_win, 5, 2, "Date:   %s", date);
        
        mvwprintw(detail_win, 7, 2, "Message:");
        wattron(detail_win, COLOR_PAIR(5));
        mvwprintw(detail_win, 8, 2, "%s", c->msg);
        wattroff(detail_win, COLOR_PAIR(5));
        
    } else if (view == 1 && selected_idx < branch_count) {
        branch_info *b = &branches[selected_idx];
        
        mvwprintw(detail_win, 3, 2, "Name: ");
        wattron(detail_win, COLOR_PAIR(b->color));
        mvwprintw(detail_win, 3, 8, "%s", b->name);
        wattroff(detail_win, COLOR_PAIR(b->color));
        
        mvwprintw(detail_win, 4, 2, "SHA: %.40s", b->oid);
        
        mvwprintw(detail_win, 5, 2, "Type: %s", b->is_remote ? "remote" : "local");
        
        if (b->is_current) {
            wattron(detail_win, COLOR_PAIR(2));
            mvwprintw(detail_win, 6, 2, "Status: CURRENT");
            wattroff(detail_win, COLOR_PAIR(2));
        }
        
    } else if (view == 2 && selected_idx < stash_count) {
        stash_info *s = &stashes[selected_idx];
        
        mvwprintw(detail_win, 3, 2, "Message: %s", s->msg);
        
        char date[64];
        struct tm *tm = localtime(&s->time);
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
        mvwprintw(detail_win, 4, 2, "Stashed: %s", date);
        
        wattron(detail_win, COLOR_PAIR(6));
        mvwprintw(detail_win, 6, 2, "Use 'bro hide pop' to apply");
        wattroff(detail_win, COLOR_PAIR(6));
        
    } else {
        wattron(detail_win, COLOR_PAIR(5));
        mvwprintw(detail_win, 3, 2, "Select an item to view details");
        wattroff(detail_win, COLOR_PAIR(5));
    }
    
    wrefresh(detail_win);
}

void draw_status() {
    werase(status_win);
    wbkgd(status_win, COLOR_PAIR(4));
    wattron(status_win, A_BOLD);
    
    const char *view_names[] = {"Commits", "Branches", "Stashes", "Remotes", "Stats"};
    mvwprintw(status_win, 0, 2, "broTUI v1.0 | %s | Selected: %d/%d",
              view_names[view], selected_idx + 1, 
              view == 0 ? commit_count : (view == 1 ? branch_count : (view == 2 ? stash_count : remote_count)));
    
    mvwprintw(status_win, 0, screen_width - 20, "[%s]", detail_visible ? "DETAIL ON" : "DETAIL OFF");
    
    wattroff(status_win, A_BOLD);
    wrefresh(status_win);
}

void draw() {
    draw_title();
    draw_sidebar();
    
    switch (view) {
        case 0: draw_commits_view(); break;
        case 1: draw_branches_view(); break;
        case 2: draw_stashes_view(); break;
        case 3: draw_remotes_view(); break;
        case 4: draw_stats_view(); break;
    }
    
    if (detail_visible) {
        draw_detail();
    }
    
    draw_status();
}

int cmd_tui(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    initscr();
    start_color();
    curs_set(0);
    keypad(stdscr, TRUE);
    noecho();
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_RED, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_BLACK, COLOR_WHITE);
    
    getmaxyx(stdscr, screen_height, screen_width);
    
    if (screen_height < 20 || screen_width < 80) {
        endwin();
        printf("broTUI requires a terminal at least 80x20\n");
        return 1;
    }
    
    int sidebar_w = 22;
    int main_w = screen_width - sidebar_w - 4;
    int detail_h = 12;
    
    title_win = newwin(1, screen_width, 0, 0);
    sidebar_win = newwin(screen_height - 4, sidebar_w, 1, 1);
    main_win = newwin(screen_height - 4, main_w, 1, sidebar_w + 1);
    detail_win = newwin(detail_h, screen_width - 2, screen_height - detail_h - 1, 1);
    status_win = newwin(1, screen_width, screen_height - 1, 0);
    
    load_branches();
    load_commits();
    
    int running = 1;
    
    while (running) {
        draw();
        
        int ch = getch();
        
        switch (ch) {
            case '1':
                view = 0;
                selected_idx = 0;
                break;
            case '2':
                view = 1;
                selected_idx = 0;
                break;
            case '3':
                view = 2;
                selected_idx = 0;
                break;
            case '4':
                view = 3;
                selected_idx = 0;
                break;
            case '5':
                view = 4;
                selected_idx = 0;
                break;
            case 'j':
            case KEY_DOWN:
                {
                    int max = view == 0 ? commit_count : (view == 1 ? branch_count : (view == 2 ? stash_count : remote_count));
                    if (selected_idx < max - 1) selected_idx++;
                }
                break;
            case 'k':
            case KEY_UP:
                if (selected_idx > 0) selected_idx--;
                break;
            case 'r':
            case 'R':
                load_branches();
                load_commits();
                break;
            case 'd':
            case 'D':
                detail_visible = !detail_visible;
                break;
            case 'q':
            case 'Q':
                running = 0;
                break;
        }
    }
    
    delwin(title_win);
    delwin(sidebar_win);
    delwin(main_win);
    delwin(detail_win);
    delwin(status_win);
    endwin();
    
    return 0;
}
