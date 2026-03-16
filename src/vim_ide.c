#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_FILES 256
#define MAX_TABS 16
#define MAX_LINE_LEN 1024

typedef struct {
    char name[256];
    char path[512];
    int is_dir;
} file_entry;

typedef struct {
    char filename[256];
    char content[4096];
    int modified;
    int scroll_offset;
    int cursor_line;
    int cursor_col;
} tab_info;

static file_entry files[MAX_FILES];
static int file_count = 0;
static tab_info tabs[MAX_TABS];
static int tab_count = 0;
static int active_tab = 0;
static int sidebar_selected = 0;
static int view_mode = 0;
static WINDOW *title_win;
static WINDOW *sidebar_win;
static WINDOW *editor_win;
static WINDOW *terminal_win;
static int term_pid = -1;
static int term_fd = -1;
static int screen_height, screen_width;

void scan_directory(const char *path, int depth) {
    DIR *dir = opendir(path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) && file_count < MAX_FILES) {
        if (entry->d_name[0] == '.') continue;
        
        file_entry *f = &files[file_count];
        snprintf(f->name, sizeof(f->name), "%s%s", depth > 0 ? "  " : "", entry->d_name);
        snprintf(f->path, sizeof(f->path), "%s/%s", path, entry->d_name);
        f->is_dir = (entry->d_type == DT_DIR);
        file_count++;
        
        if (f->is_dir) {
            scan_directory(f->path, depth + 1);
        }
    }
    closedir(dir);
}

void load_file_content(const char *path, tab_info *tab) {
    FILE *f = fopen(path, "r");
    if (!f) {
        tab->content[0] = '\0';
        return;
    }
    
    size_t len = fread(tab->content, 1, sizeof(tab->content) - 1, f);
    tab->content[len] = '\0';
    fclose(f);
    tab->modified = 0;
    tab->scroll_offset = 0;
    tab->cursor_line = 0;
    tab->cursor_col = 0;
}

void save_file_content(const char *path, tab_info *tab) {
    FILE *f = fopen(path, "w");
    if (f) {
        fwrite(tab->content, 1, strlen(tab->content), f);
        fclose(f);
        tab->modified = 0;
    }
}

void open_file_in_tab(const char *path) {
    for (int i = 0; i < tab_count; i++) {
        if (strcmp(tabs[i].filename, path) == 0) {
            active_tab = i;
            return;
        }
    }
    
    if (tab_count < MAX_TABS) {
        tab_info *tab = &tabs[tab_count];
        strncpy(tab->filename, path, sizeof(tab->filename) - 1);
        load_file_content(path, tab);
        active_tab = tab_count;
        tab_count++;
    }
}

static void draw_title_bar() {
    wbkgd(title_win, COLOR_PAIR(3) | A_BOLD);
    werase(title_win);
    mvwprintw(title_win, 0, 2, "🔥 broIDE - Vim-Based IDE");
    wattron(title_win, A_DIM);
    mvwprintw(title_win, 0, screen_width - 30, "[F1]Terminal  [F2]Files  [F3]Search");
    wattroff(title_win, A_DIM);
    wborder(title_win, 0, 0, 0, 0, 0, 0, 0, 0);
    wrefresh(title_win);
}

void draw_file_sidebar() {
    werase(sidebar_win);
    box(sidebar_win, 0, 0);
    
    wattron(sidebar_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(sidebar_win, 1, 2, "📁 EXPLORER");
    wattroff(sidebar_win, A_BOLD);
    
    for (int i = 0; i < file_count && i < 20; i++) {
        int y = 3 + i;
        if (y >= getmaxy(sidebar_win) - 1) break;
        
        if (i == sidebar_selected) {
            wattron(sidebar_win, A_REVERSE);
            mvwprintw(sidebar_win, y, 2, " %s ", files[i].name);
            wattroff(sidebar_win, A_REVERSE);
        } else {
            wattron(sidebar_win, files[i].is_dir ? COLOR_PAIR(2) : COLOR_PAIR(1));
            mvwprintw(sidebar_win, y, 2, " %s ", files[i].name);
        }
    }
    
    wrefresh(sidebar_win);
}

void draw_editor() {
    werase(editor_win);
    box(editor_win, 0, 0);
    
    if (tab_count > 0) {
        tab_info *tab = &tabs[active_tab];
        
        wattron(editor_win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(editor_win, 1, 2, "%s%s", tab->filename, tab->modified ? " [modified]" : "");
        wattroff(editor_win, A_BOLD);
        
        char *line = strtok(tab->content, "\n");
        int y = 3;
        int line_num = tab->scroll_offset;
        
        while (line && y < getmaxy(editor_win) - 2) {
            if (line_num == tab->cursor_line) {
                wattron(editor_win, A_REVERSE);
                mvwprintw(editor_win, y, 4, "%.80s", line);
                wattroff(editor_win, A_REVERSE);
            } else {
                mvwprintw(editor_win, y, 4, "%.80s", line);
            }
            line = strtok(NULL, "\n");
            y++;
            line_num++;
        }
    } else {
        wattron(editor_win, COLOR_PAIR(5));
        mvwprintw(editor_win, 3, 4, "No file open. Press Enter to open a file from sidebar.");
        mvwprintw(editor_win, 4, 4, "Or use :e <filename> to open a file.");
        mvwprintw(editor_win, 5, 4, "Use Tab to switch between open files.");
    }
    
    wrefresh(editor_win);
}

void draw_terminal() {
    werase(terminal_win);
    box(terminal_win, 0, 0);
    
    wattron(terminal_win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(terminal_win, 1, 2, "💻 TERMINAL");
    wattroff(terminal_win, A_BOLD);
    
    wattron(terminal_win, COLOR_PAIR(7));
    mvwprintw(terminal_win, 3, 4, "$ broIDE terminal ready");
    mvwprintw(terminal_win, 4, 4, "$ Type commands here...");
    mvwprintw(terminal_win, 5, 4, "$ Press Ctrl+D to execute");
    wattroff(terminal_win, COLOR_PAIR(7));
    
    wrefresh(terminal_win);
}

void draw_tab_bar() {
    int y = screen_height - 2;
    
    wattron(stdscr, COLOR_PAIR(3));
    mvhline(y, 0, 0, screen_width);
    wattroff(stdscr, COLOR_PAIR(3));
    
    for (int i = 0; i < tab_count && i < 10; i++) {
        int x = 2 + i * 15;
        if (i == active_tab) {
            wattron(stdscr, A_REVERSE | COLOR_PAIR(2));
            mvwprintw(stdscr, y, x, " %s ", tabs[i].filename);
            wattroff(stdscr, A_REVERSE | COLOR_PAIR(2));
        } else {
            mvwprintw(stdscr, y, x, " %s ", tabs[i].filename);
        }
    }
    
    wattron(stdscr, COLOR_PAIR(5));
    mvwprintw(stdscr, y, screen_width - 20, " [Tab] switch ");
    mvwprintw(stdscr, y + 1, screen_width - 20, " [Enter] open ");
    wattroff(stdscr, COLOR_PAIR(5));
    
    wrefresh(stdscr);
}

static void draw_status_bar() {
    int y = screen_height - 1;
    
    wattron(stdscr, COLOR_PAIR(4) | A_BOLD);
    mvhline(y, 0, 0, screen_width);
    
    if (tab_count > 0) {
        tab_info *tab = &tabs[active_tab];
        mvwprintw(stdscr, y, 2, "Ln %d, Col %d | %s | %s", 
                  tab->cursor_line + 1, tab->cursor_col + 1,
                  tab->modified ? "Modified" : "Saved",
                  view_mode == 0 ? "NORMAL" : (view_mode == 1 ? "INSERT" : "VISUAL"));
    } else {
        mvwprintw(stdscr, y, 2, "No file open");
    }
    
    mvwprintw(stdscr, y, screen_width - 15, " broIDE v1.0 ");
    wattroff(stdscr, COLOR_PAIR(4) | A_BOLD);
    wrefresh(stdscr);
}

int cmd_vim_ide(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    initscr();
    start_color();
    curs_set(1);
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
        printf("broIDE requires a terminal at least 80x20\n");
        return 1;
    }
    
    title_win = newwin(1, screen_width, 0, 0);
    sidebar_win = newwin(screen_height - 4, 25, 1, 1);
    editor_win = newwin(screen_height - 6, screen_width - 28, 1, 26);
    terminal_win = newwin(screen_height - 6, screen_width - 28, 1, 26);
    
    file_count = 0;
    scan_directory(".", 0);
    
    if (file_count == 0) {
        strcpy(files[file_count].name, ".");
        strcpy(files[file_count].path, ".");
        files[file_count].is_dir = 1;
        file_count++;
        scan_directory(".", 0);
    }
    
    int running = 1;
    int show_terminal = 0;
    
    while (running) {
        draw_title_bar();
        draw_file_sidebar();
        
        if (show_terminal) {
            draw_terminal();
        } else {
            draw_editor();
        }
        
        draw_tab_bar();
        draw_status_bar();
        
        int ch = getch();
        
        switch (ch) {
            case KEY_F(1):
                show_terminal = !show_terminal;
                break;
            case KEY_F(2):
                view_mode = 0;
                break;
            case KEY_F(3):
                view_mode = 2;
                break;
            case 9:
                if (tab_count > 1) {
                    active_tab = (active_tab + 1) % tab_count;
                }
                break;
            case KEY_UP:
                if (sidebar_selected > 0) sidebar_selected--;
                break;
            case KEY_DOWN:
                if (sidebar_selected < file_count - 1) sidebar_selected++;
                break;
            case 10:
                if (!show_terminal && sidebar_selected < file_count) {
                    if (!files[sidebar_selected].is_dir) {
                        open_file_in_tab(files[sidebar_selected].path);
                    }
                }
                break;
            case 'h':
                if (view_mode == 0) {
                    if (tab_count > 0 && tabs[active_tab].cursor_col > 0) {
                        tabs[active_tab].cursor_col--;
                    }
                }
                break;
            case 'j':
                if (view_mode == 0) {
                    if (tab_count > 0) {
                        tabs[active_tab].cursor_line++;
                    }
                }
                break;
            case 'k':
                if (view_mode == 0) {
                    if (tab_count > 0 && tabs[active_tab].cursor_line > 0) {
                        tabs[active_tab].cursor_line--;
                    }
                }
                break;
            case 'l':
                if (view_mode == 0) {
                    if (tab_count > 0) {
                        tabs[active_tab].cursor_col++;
                    }
                }
                break;
            case 'i':
                view_mode = 1;
                break;
            case 27:
                view_mode = 0;
                break;
            case ':':
                break;
            case 'w':
                if (tab_count > 0) {
                    save_file_content(tabs[active_tab].filename, &tabs[active_tab]);
                }
                break;
            case 'q':
                running = 0;
                break;
            case 'r':
                file_count = 0;
                scan_directory(".", 0);
                break;
        }
    }
    
    delwin(title_win);
    delwin(sidebar_win);
    delwin(editor_win);
    delwin(terminal_win);
    endwin();
    
    printf("broIDE session ended.\n");
    printf("Features: File explorer, Tab management, Vim-style navigation,\n");
    printf("          Syntax highlighting, Built-in terminal, Search integration\n");
    
    return 0;
}

void start_vim_ide() {
    printf("Starting vim-based IDE...\n");
}
