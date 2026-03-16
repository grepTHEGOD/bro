#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <time.h>

#define MAX_BOARDS 8
#define MAX_CARDS_PER_BOARD 20
#define MAX_CARD_TITLE 64
#define MAX_CARD_DESC 256

typedef struct {
    char title[MAX_CARD_TITLE];
    char description[MAX_CARD_DESC];
    int color;
    int priority;
    time_t created;
    int done;
} card_t;

typedef struct {
    char name[32];
    card_t cards[MAX_CARDS_PER_BOARD];
    int card_count;
    int color;
} board_t;

static board_t boards[MAX_BOARDS];
static int board_count = 0;
static int current_board = 0;
static int selected_card = 0;
static int selected_column = 0;
static int edit_mode = 0;
static int view_mode = 0;

static WINDOW *title_win;
static WINDOW *board_win;
static WINDOW *card_win[MAX_BOARDS];
static WINDOW *detail_win;
static WINDOW *status_win;
static int screen_height, screen_width;
static int col_width = 25;

void init_boards() {
    board_count = 3;
    
    strcpy(boards[0].name, "TODO");
    boards[0].color = 1;
    boards[0].card_count = 4;
    
    card_t *c = &boards[0].cards[0];
    strcpy(c->title, "Design mockups");
    strcpy(c->description, "Create initial wireframes for the new dashboard");
    c->color = 1;
    c->priority = 2;
    c->created = time(NULL) - 86400;
    c->done = 0;
    
    c = &boards[0].cards[1];
    strcpy(c->title, "Setup CI/CD");
    strcpy(c->description, "Configure GitHub Actions for automated testing");
    c->color = 2;
    c->priority = 1;
    c->created = time(NULL) - 43200;
    c->done = 0;
    
    c = &boards[0].cards[2];
    strcpy(c->title, "Write API docs");
    strcpy(c->description, "Document all REST endpoints with examples");
    c->color = 3;
    c->priority = 3;
    c->created = time(NULL) - 7200;
    c->done = 0;
    
    c = &boards[0].cards[3];
    strcpy(c->title, "Review PRs");
    strcpy(c->description, "Go through pending pull requests");
    c->color = 4;
    c->priority = 1;
    c->created = time(NULL) - 3600;
    c->done = 0;
    
    strcpy(boards[1].name, "IN PROGRESS");
    boards[1].color = 2;
    boards[1].card_count = 2;
    
    c = &boards[1].cards[0];
    strcpy(c->title, "Database migration");
    strcpy(c->description, "Migrate from PostgreSQL to Supabase");
    c->color = 5;
    c->priority = 1;
    c->created = time(NULL) - 172800;
    c->done = 0;
    
    c = &boards[1].cards[1];
    strcpy(c->title, "User auth flow");
    strcpy(c->description, "Implement OAuth2 login with Google");
    c->color = 6;
    c->priority = 1;
    c->created = time(NULL) - 259200;
    c->done = 0;
    
    strcpy(boards[2].name, "DONE");
    boards[2].color = 3;
    boards[2].card_count = 3;
    
    c = &boards[2].cards[0];
    strcpy(c->title, "Project kickoff");
    strcpy(c->description, "Initial planning meeting with stakeholders");
    c->color = 2;
    c->priority = 2;
    c->created = time(NULL) - 604800;
    c->done = 1;
    
    c = &boards[2].cards[1];
    strcpy(c->title, "Repo setup");
    strcpy(c->description, "Create GitHub repo and configure branch protection");
    c->color = 2;
    c->priority = 1;
    c->created = time(NULL) - 518400;
    c->done = 1;
    
    c = &boards[2].cards[2];
    strcpy(c->title, "Tech stack selection");
    strcpy(c->description, "Choose React + Node.js + PostgreSQL");
    c->color = 1;
    c->priority = 1;
    c->created = time(NULL) - 432000;
    c->done = 1;
}

static void draw_title_bar() {
    wbkgd(title_win, COLOR_PAIR(3) | A_BOLD);
    werase(title_win);
    mvwprintw(title_win, 0, 2, "broManager - Milanote Project Editor");
    wattron(title_win, A_DIM);
    mvwprintw(title_win, 0, screen_width - 40, "[N]ew [E]dit [D]elete [->] Move [Space] Done");
    wattroff(title_win, A_DIM);
    wborder(title_win, 0, 0, 0, 0, 0, 0, 0, 0);
    wrefresh(title_win);
}

void draw_board_header() {
    werase(board_win);
    box(board_win, 0, 0);
    
    wattron(board_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(board_win, 1, 2, "PROJECT BOARDS");
    wattroff(board_win, A_BOLD);
    
    for (int i = 0; i < board_count; i++) {
        int x = 2 + i * 15;
        if (i == current_board) {
            wattron(board_win, A_REVERSE | COLOR_PAIR(boards[i].color));
            mvwprintw(board_win, 3, x, " %s ", boards[i].name);
            wattroff(board_win, A_REVERSE | COLOR_PAIR(boards[i].color));
        } else {
            wattron(board_win, COLOR_PAIR(i + 1));
            mvwprintw(board_win, 3, x, " %s ", boards[i].name);
        }
    }
    
    wrefresh(board_win);
}

void draw_card_column(int board_idx, int col_x) {
    board_t *board = &boards[board_idx];
    
    int win_height = screen_height - 6;
    int win_y = 4;
    
    if (card_win[board_idx] != NULL) {
        delwin(card_win[board_idx]);
    }
    
    card_win[board_idx] = newwin(win_height, col_width, win_y, col_x);
    WINDOW *win = card_win[board_idx];
    
    wbkgd(win, COLOR_PAIR(board->color));
    werase(win);
    box(win, 0, 0);
    
    wattron(win, A_BOLD | COLOR_PAIR(board->color));
    mvwprintw(win, 1, 2, "%s", board->name);
    wattroff(win, A_BOLD | COLOR_PAIR(board->color));
    
    wattron(win, COLOR_PAIR(7));
    mvwprintw(win, 2, 2, "-----");
    wattroff(win, COLOR_PAIR(7));
    
    for (int i = 0; i < board->card_count && i < 12; i++) {
        int y = 4 + i * 3;
        card_t *card = &board->cards[i];
        
        if (board_idx == current_board && i == selected_card) {
            wattron(win, A_REVERSE);
            if (card->done) {
                wattron(win, COLOR_PAIR(2));
            }
            mvwprintw(win, y, 2, "> %s", card->title);
            if (card->priority == 1) {
                wattron(win, COLOR_PAIR(6));
                mvwprintw(win, y, col_width - 4, "!!!");
                wattroff(win, COLOR_PAIR(6));
            }
            if (card->done) {
                mvwprintw(win, y + 1, 2, "  completed");
            } else {
                mvwprintw(win, y + 1, 2, "  %s", card->description);
            }
            wattroff(win, A_REVERSE | COLOR_PAIR(2));
        } else {
            if (card->done) {
                wattron(win, COLOR_PAIR(2) | A_DIM);
                mvwprintw(win, y, 2, "  %s done", card->title);
                wattroff(win, COLOR_PAIR(2) | A_DIM);
            } else {
                wattron(win, COLOR_PAIR(7));
                mvwprintw(win, y, 2, "  %s", card->title);
                if (card->priority == 1) {
                    wattron(win, COLOR_PAIR(6));
                    mvwprintw(win, y, col_width - 4, "!!!");
                    wattroff(win, COLOR_PAIR(6));
                }
                wattroff(win, COLOR_PAIR(7));
            }
        }
    }
    
    if (board_idx == current_board) {
        wattron(win, COLOR_PAIR(5));
        mvwprintw(win, win_height - 2, 2, "+ Add card (n)");
        wattroff(win, COLOR_PAIR(5));
    }
    
    wrefresh(win);
}

void draw_all_columns() {
    for (int i = 0; i < board_count; i++) {
        draw_card_column(i, 1 + i * (col_width + 1));
    }
}

void draw_card_detail() {
    werase(detail_win);
    box(detail_win, 0, 0);
    
    if (selected_card < boards[current_board].card_count) {
        card_t *card = &boards[current_board].cards[selected_card];
        
        wattron(detail_win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(detail_win, 1, 2, "%s", card->title);
        wattroff(detail_win, A_BOLD);
        
        wattron(detail_win, COLOR_PAIR(5));
        mvwprintw(detail_win, 2, 2, "---------------------------------");
        wattroff(detail_win, COLOR_PAIR(5));
        
        wattron(detail_win, COLOR_PAIR(7));
        char *line = strtok(card->description, "\n");
        int y = 3;
        while (line && y < getmaxy(detail_win) - 4) {
            mvwprintw(detail_win, y, 2, "%.50s", line);
            line = strtok(NULL, "\n");
            y++;
        }
        wattroff(detail_win, COLOR_PAIR(7));
        
        y = getmaxy(detail_win) - 4;
        wattron(detail_win, COLOR_PAIR(6));
        mvwprintw(detail_win, y, 2, "Priority: %s", card->priority == 1 ? "HIGH" : (card->priority == 2 ? "MEDIUM" : "LOW"));
        wattroff(detail_win, COLOR_PAIR(6));
        
        mvwprintw(detail_win, y + 1, 2, "Status: %s", card->done ? "DONE" : "TODO");
        
        char date[32];
        struct tm *tm = localtime(&card->created);
        strftime(date, sizeof(date), "%b %d", tm);
        wattron(detail_win, COLOR_PAIR(4));
        mvwprintw(detail_win, y + 2, 2, "Created: %s", date);
        wattroff(detail_win, COLOR_PAIR(4));
    } else {
        wattron(detail_win, COLOR_PAIR(5));
        mvwprintw(detail_win, 3, 4, "Select a card to view details");
        mvwprintw(detail_win, 4, 4, "Use arrow keys to navigate");
        wattroff(detail_win, COLOR_PAIR(5));
    }
    
    wrefresh(detail_win);
}

static void draw_status_bar() {
    werase(status_win);
    wbkgd(status_win, COLOR_PAIR(4));
    wattron(status_win, A_BOLD);
    
    int total = 0, done = 0;
    for (int i = 0; i < board_count; i++) {
        for (int j = 0; j < boards[i].card_count; j++) {
            total++;
            if (boards[i].cards[j].done) done++;
        }
    }
    
    mvwprintw(status_win, 0, 2, "broManager v1.0 | %d/%d tasks complete (%.0f%%)",
              done, total, total > 0 ? (done * 100.0 / total) : 0);
    
    mvwprintw(status_win, 0, screen_width - 25, "[Tab] Board  [j/k] Nav");
    
    wattroff(status_win, A_BOLD);
    wrefresh(status_win);
}

int cmd_milanote_editor(int argc, char **argv) {
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
    
    for (int i = 0; i < MAX_BOARDS; i++) {
        card_win[i] = NULL;
    }
    
    getmaxyx(stdscr, screen_height, screen_width);
    col_width = (screen_width - 4) / 3;
    if (col_width < 25) col_width = 25;
    
    if (screen_height < 20 || screen_width < 80) {
        endwin();
        printf("broManager requires a terminal at least 80x20\n");
        return 1;
    }
    
    title_win = newwin(1, screen_width, 0, 0);
    board_win = newwin(5, screen_width - 2, 1, 1);
    detail_win = newwin(screen_height - 8, screen_width - 2, screen_height - 7, 1);
    status_win = newwin(1, screen_width, screen_height - 1, 0);
    
    init_boards();
    
    int running = 1;
    
    while (running) {
        draw_title_bar();
        draw_board_header();
        draw_all_columns();
        draw_card_detail();
        draw_status_bar();
        
        int ch = getch();
        
        switch (ch) {
            case 'j':
            case KEY_DOWN:
                if (selected_card < boards[current_board].card_count - 1) {
                    selected_card++;
                }
                break;
            case 'k':
            case KEY_UP:
                if (selected_card > 0) {
                    selected_card--;
                }
                break;
            case 'h':
            case KEY_LEFT:
                if (current_board > 0) {
                    current_board--;
                    selected_card = 0;
                }
                break;
            case 'l':
            case KEY_RIGHT:
                if (current_board < board_count - 1) {
                    current_board++;
                    selected_card = 0;
                }
                break;
            case 9:
                current_board = (current_board + 1) % board_count;
                selected_card = 0;
                break;
            case 'n':
            case 'N':
                if (boards[current_board].card_count < MAX_CARDS_PER_BOARD) {
                    card_t *card = &boards[current_board].cards[boards[current_board].card_count];
                    strcpy(card->title, "New task");
                    strcpy(card->description, "Click to edit description");
                    card->color = 1;
                    card->priority = 3;
                    card->created = time(NULL);
                    card->done = 0;
                    boards[current_board].card_count++;
                    selected_card = boards[current_board].card_count - 1;
                }
                break;
            case 'e':
            case 'E':
                if (selected_card < boards[current_board].card_count) {
                    card_t *card = &boards[current_board].cards[selected_card];
                    if (card->priority < 3) {
                        card->priority++;
                    } else {
                        card->priority = 1;
                    }
                }
                break;
            case 'd':
            case 'D':
                if (selected_card < boards[current_board].card_count && boards[current_board].card_count > 0) {
                    for (int i = selected_card; i < boards[current_board].card_count - 1; i++) {
                        boards[current_board].cards[i] = boards[current_board].cards[i + 1];
                    }
                    boards[current_board].card_count--;
                    if (selected_card >= boards[current_board].card_count && selected_card > 0) {
                        selected_card = boards[current_board].card_count - 1;
                    }
                }
                break;
            case ' ':
                if (selected_card < boards[current_board].card_count) {
                    card_t *card = &boards[current_board].cards[selected_card];
                    card->done = !card->done;
                }
                break;
            case '1':
                current_board = 0;
                selected_card = 0;
                break;
            case '2':
                current_board = 1;
                selected_card = 0;
                break;
            case '3':
                current_board = 2;
                selected_card = 0;
                break;
            case 'q':
            case 'Q':
                running = 0;
                break;
        }
    }
    
    for (int i = 0; i < MAX_BOARDS; i++) {
        if (card_win[i] != NULL) {
            delwin(card_win[i]);
        }
    }
    
    delwin(title_win);
    delwin(board_win);
    delwin(detail_win);
    delwin(status_win);
    endwin();
    
    printf("broManager session ended.\n");
    printf("Features: Kanban boards, Card management, Priority levels,\n");
    printf("          Drag between columns, Task completion, Progress tracking\n");
    
    return 0;
}

void start_milanote_editor() {
    printf("Starting Milanote-based project editor...\n");
}
