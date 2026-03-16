#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <time.h>

#define MAX_EMAILS 100
#define MAX_SUBJECTS 64

typedef struct {
    char id[32];
    char sender[64];
    char recipient[64];
    char subject[128];
    char body[1024];
    time_t timestamp;
    int read;
    int starred;
    int flagged;
    int has_attach;
} email_t;

typedef struct {
    char name[32];
    char path[64];
    int unread;
} folder_t;

static email_t emails[MAX_EMAILS];
static int email_count = 0;
static folder_t folders[8];
static int folder_count = 0;
static int selected_email = 0;
static int current_folder = 0;
static int view_mode = 0;
static int compose_mode = 0;
static char compose_to[64] = {0};
static char compose_subject[128] = {0};
static char compose_body[512] = {0};
static int compose_field = 0;

static WINDOW *title_win;
static WINDOW *folder_win;
static WINDOW *list_win;
static WINDOW *preview_win;
static WINDOW *status_win;
static int screen_height, screen_width;

void init_folders() {
    folder_count = 0;
    
    strcpy(folders[folder_count].name, "Inbox");
    strcpy(folders[folder_count].path, "INBOX");
    folders[folder_count].unread = 3;
    folder_count++;
    
    strcpy(folders[folder_count].name, "Sent");
    strcpy(folders[folder_count].path, "SENT");
    folders[folder_count].unread = 0;
    folder_count++;
    
    strcpy(folders[folder_count].name, "Drafts");
    strcpy(folders[folder_count].path, "DRAFTS");
    folders[folder_count].unread = 1;
    folder_count++;
    
    strcpy(folders[folder_count].name, "Trash");
    strcpy(folders[folder_count].path, "TRASH");
    folders[folder_count].unread = 0;
    folder_count++;
    
    strcpy(folders[folder_count].name, "Archive");
    strcpy(folders[folder_count].path, "ARCHIVE");
    folders[folder_count].unread = 0;
    folder_count++;
    
    strcpy(folders[folder_count].name, "Junk");
    strcpy(folders[folder_count].path, "JUNK");
    folders[folder_count].unread = 2;
    folder_count++;
}

void load_emails() {
    email_count = 0;
    
    email_t *e = &emails[email_count];
    strcpy(e->id, "1001");
    strcpy(e->sender, "boss@company.com");
    strcpy(e->recipient, "me@localhost");
    strcpy(e->subject, "Project deadline update");
    strcpy(e->body, "Hey,\n\nJust a quick reminder that the project deadline has been moved to next Friday. Please make sure to get the latest changes in before then.\n\nThanks!");
    e->timestamp = time(NULL) - 3600;
    e->read = 0;
    e->starred = 1;
    e->flagged = 0;
    e->has_attach = 1;
    email_count++;
    
    e = &emails[email_count];
    strcpy(e->id, "1002");
    strcpy(e->sender, "github@noreply.com");
    strcpy(e->recipient, "me@localhost");
    strcpy(e->subject, "[bro] New issue comment on #42");
    strcpy(e->body, "Someone commented on your issue:\n\n\"Hey, I think I found a fix for this!\"\n\nView it on GitHub: https://github.com/...");
    e->timestamp = time(NULL) - 7200;
    e->read = 1;
    e->starred = 0;
    e->flagged = 0;
    e->has_attach = 0;
    email_count++;
    
    e = &emails[email_count];
    strcpy(e->id, "1003");
    strcpy(e->sender, "team@dev.io");
    strcpy(e->recipient, "me@localhost");
    strcpy(e->subject, "Code review request");
    strcpy(e->body, "Hi,\n\nCould you review my PR when you get a chance? It's for the new authentication feature.\n\nPR #156\n\nThanks!");
    e->timestamp = time(NULL) - 14400;
    e->read = 0;
    e->starred = 0;
    e->flagged = 1;
    e->has_attach = 0;
    email_count++;
    
    e = &emails[email_count];
    strcpy(e->id, "1004");
    strcpy(e->sender, "newsletter@rust-lang.org");
    strcpy(e->recipient, "me@localhost");
    strcpy(e->subject, "This Week in Rust #527");
    strcpy(e->body, "Hello Rustaceans!\n\nThis week in Rust we've had many exciting updates...\n\nCheck out the full newsletter for more details!");
    e->timestamp = time(NULL) - 86400;
    e->read = 1;
    e->starred = 0;
    e->flagged = 0;
    e->has_attach = 0;
    email_count++;
    
    e = &emails[email_count];
    strcpy(e->id, "1005");
    strcpy(e->sender, "alerts@monitoring.com");
    strcpy(e->recipient, "me@localhost");
    strcpy(e->subject, "Server CPU usage alert");
    strcpy(e->body, "ALERT: Server prod-server-01 CPU usage exceeded 90%\n\nCurrent: 94%\nDuration: 5 minutes\n\nLog in to dashboard to investigate.");
    e->timestamp = time(NULL) - 300;
    e->read = 0;
    e->starred = 1;
    e->flagged = 1;
    e->has_attach = 0;
    email_count++;
}

const char *format_time(time_t t) {
    static char buf[32];
    time_t now = time(NULL);
    int diff = now - t;
    
    if (diff < 60) {
        snprintf(buf, sizeof(buf), "now");
    } else if (diff < 3600) {
        snprintf(buf, sizeof(buf), "%dm ago", diff / 60);
    } else if (diff < 86400) {
        snprintf(buf, sizeof(buf), "%dh ago", diff / 3600);
    } else {
        struct tm *tm = localtime(&t);
        strftime(buf, sizeof(buf), "%b %d", tm);
    }
    return buf;
}

static void draw_title_bar() {
    wbkgd(title_win, COLOR_PAIR(3) | A_BOLD);
    werase(title_win);
    mvwprintw(title_win, 0, 2, "📧 broMail - Thunderbird Terminal");
    wattron(title_win, A_DIM);
    mvwprintw(title_win, 0, screen_width - 35, "[N]ew [R]eply [D]elete [F]lag [S]tar [C]ompose");
    wattroff(title_win, A_DIM);
    wborder(title_win, 0, 0, 0, 0, 0, 0, 0, 0);
    wrefresh(title_win);
}

void draw_folders() {
    werase(folder_win);
    box(folder_win, 0, 0);
    
    wattron(folder_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(folder_win, 1, 2, "📂 FOLDERS");
    wattroff(folder_win, A_BOLD);
    
    for (int i = 0; i < folder_count; i++) {
        int y = 3 + i * 2;
        if (y >= getmaxy(folder_win) - 2) break;
        
        if (i == current_folder) {
            wattron(folder_win, A_REVERSE | COLOR_PAIR(2));
            mvwprintw(folder_win, y, 2, " %s ", folders[i].name);
            if (folders[i].unread > 0) {
                wattron(folder_win, COLOR_PAIR(6));
                mvwprintw(folder_win, y, 18, "(%d)", folders[i].unread);
                wattroff(folder_win, COLOR_PAIR(6));
            }
            wattroff(folder_win, A_REVERSE | COLOR_PAIR(2));
        } else {
            wattron(folder_win, COLOR_PAIR(i == 0 ? 1 : 7));
            mvwprintw(folder_win, y, 2, " %s ", folders[i].name);
            if (folders[i].unread > 0) {
                wattron(folder_win, COLOR_PAIR(6));
                mvwprintw(folder_win, y, 18, "(%d)", folders[i].unread);
                wattroff(folder_win, COLOR_PAIR(6));
            }
        }
    }
    
    wrefresh(folder_win);
}

void draw_email_list() {
    werase(list_win);
    box(list_win, 0, 0);
    
    wattron(list_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(list_win, 1, 2, "📬 %s (%d)", folders[current_folder].name, email_count);
    wattroff(list_win, A_BOLD);
    
    for (int i = 0; i < email_count && i < 15; i++) {
        int y = 3 + i;
        if (y >= getmaxy(list_win) - 2) break;
        
        email_t *e = &emails[i];
        
        if (i == selected_email) {
            wattron(list_win, A_REVERSE);
        } else if (!e->read) {
            wattron(list_win, A_BOLD);
        }
        
        char flag_char = ' ';
        if (e->flagged) flag_char = '!';
        else if (e->starred) flag_char = '*';
        else if (e->has_attach) flag_char = '+';
        
        mvwprintw(list_win, y, 2, "%c %-20.20s %-30.30s %s", 
                  flag_char, e->sender, e->subject, format_time(e->timestamp));
        
        if (i == selected_email) {
            wattroff(list_win, A_REVERSE);
        } else if (!e->read) {
            wattroff(list_win, A_BOLD);
        }
    }
    
    wrefresh(list_win);
}

void draw_preview() {
    werase(preview_win);
    box(preview_win, 0, 0);
    
    if (selected_email < email_count) {
        email_t *e = &emails[selected_email];
        
        wattron(preview_win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(preview_win, 1, 2, "From: %s", e->sender);
        mvwprintw(preview_win, 2, 2, "To: %s", e->recipient);
        mvwprintw(preview_win, 3, 2, "Subject: %s", e->subject);
        wattroff(preview_win, A_BOLD);
        
        char date[64];
        struct tm *tm = localtime(&e->timestamp);
        strftime(date, sizeof(date), "%a, %b %d, %Y %H:%M", tm);
        wattron(preview_win, COLOR_PAIR(5));
        mvwprintw(preview_win, 4, 2, "Date: %s", date);
        wattroff(preview_win, COLOR_PAIR(5));
        
        mvwprintw(preview_win, 5, 2, "────────────────────────────────────────");
        
        wattron(preview_win, COLOR_PAIR(7));
        char *line = strtok(e->body, "\n");
        int y = 6;
        while (line && y < getmaxy(preview_win) - 2) {
            mvwprintw(preview_win, y, 2, "%.70s", line);
            line = strtok(NULL, "\n");
            y++;
        }
        wattroff(preview_win, COLOR_PAIR(7));
    } else {
        wattron(preview_win, COLOR_PAIR(5));
        mvwprintw(preview_win, 3, 4, "No email selected. Use arrow keys to navigate.");
        wattroff(preview_win, COLOR_PAIR(5));
    }
    
    wrefresh(preview_win);
}

void draw_compose() {
    werase(preview_win);
    box(preview_win, 0, 0);
    
    wattron(preview_win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(preview_win, 1, 2, "✎ COMPOSE NEW EMAIL");
    wattroff(preview_win, A_BOLD);
    
    int y = 3;
    mvwprintw(preview_win, y, 2, "To: ");
    if (compose_field == 0) wattron(preview_win, A_REVERSE);
    mvwprintw(preview_win, y, 7, "%-50s", compose_to);
    if (compose_field == 0) wattroff(preview_win, A_REVERSE);
    
    y++;
    mvwprintw(preview_win, y, 2, "Sub:");
    if (compose_field == 1) wattron(preview_win, A_REVERSE);
    mvwprintw(preview_win, y, 7, "%-50s", compose_subject);
    if (compose_field == 1) wattroff(preview_win, A_REVERSE);
    
    y += 2;
    mvwprintw(preview_win, y, 2, "Body:");
    if (compose_field == 2) wattron(preview_win, A_REVERSE);
    mvwprintw(preview_win, y, 8, "%-50s", compose_body);
    if (compose_field == 2) wattroff(preview_win, A_REVERSE);
    
    y += 2;
    wattron(preview_win, COLOR_PAIR(6));
    mvwprintw(preview_win, y, 2, "[Enter] Send  [Tab] Next Field  [Esc] Cancel");
    wattroff(preview_win, COLOR_PAIR(6));
    
    wrefresh(preview_win);
}

static void draw_status_bar() {
    werase(status_win);
    wbkgd(status_win, COLOR_PAIR(4));
    wattron(status_win, A_BOLD);
    
    mvwprintw(status_win, 0, 2, "broMail v1.0 | %d emails | %s", 
              email_count, folders[current_folder].name);
    
    if (selected_email < email_count) {
        email_t *e = &emails[selected_email];
        mvwprintw(status_win, 0, screen_width - 40, "[%s] %s",
                  e->read ? "READ" : "UNREAD",
                  e->flagged ? "FLAGGED" : (e->starred ? "STARRED" : ""));
    }
    
    wattroff(status_win, A_BOLD);
    wrefresh(status_win);
}

int cmd_thunderbird_email(int argc, char **argv) {
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
    
    getmaxyx(stdscr, screen_height, screen_width);
    
    if (screen_height < 20 || screen_width < 80) {
        endwin();
        printf("broMail requires a terminal at least 80x20\n");
        return 1;
    }
    
    title_win = newwin(1, screen_width, 0, 0);
    folder_win = newwin(18, 22, 1, 1);
    list_win = newwin(18, screen_width - 24, 1, 22);
    preview_win = newwin(screen_height - 21, screen_width - 2, 19, 1);
    status_win = newwin(1, screen_width, screen_height - 1, 0);
    
    init_folders();
    load_emails();
    
    int running = 1;
    
    while (running) {
        draw_title_bar();
        draw_folders();
        
        if (compose_mode) {
            draw_compose();
        } else {
            draw_email_list();
            draw_preview();
        }
        
        draw_status_bar();
        
        int ch = getch();
        
        if (compose_mode) {
            switch (ch) {
                case 27:
                    compose_mode = 0;
                    compose_field = 0;
                    break;
                case 9:
                    compose_field = (compose_field + 1) % 3;
                    break;
                case 10:
                    if (compose_field == 2) {
                        compose_mode = 0;
                        memset(compose_to, 0, sizeof(compose_to));
                        memset(compose_subject, 0, sizeof(compose_subject));
                        memset(compose_body, 0, sizeof(compose_body));
                        compose_field = 0;
                    }
                    break;
                case KEY_BACKSPACE:
                case 127:
                    if (compose_field == 0 && strlen(compose_to) > 0) {
                        compose_to[strlen(compose_to) - 1] = '\0';
                    } else if (compose_field == 1 && strlen(compose_subject) > 0) {
                        compose_subject[strlen(compose_subject) - 1] = '\0';
                    } else if (compose_field == 2 && strlen(compose_body) > 0) {
                        compose_body[strlen(compose_body) - 1] = '\0';
                    }
                    break;
                default:
                    if (ch >= 32 && ch <= 126) {
                        if (compose_field == 0 && strlen(compose_to) < 63) {
                            int len = strlen(compose_to);
                            compose_to[len] = ch;
                            compose_to[len + 1] = '\0';
                        } else if (compose_field == 1 && strlen(compose_subject) < 127) {
                            int len = strlen(compose_subject);
                            compose_subject[len] = ch;
                            compose_subject[len + 1] = '\0';
                        } else if (compose_field == 2 && strlen(compose_body) < 511) {
                            int len = strlen(compose_body);
                            compose_body[len] = ch;
                            compose_body[len + 1] = '\0';
                        }
                    }
                    break;
            }
        } else {
            switch (ch) {
                case 'c':
                case 'C':
                    compose_mode = 1;
                    compose_field = 0;
                    break;
                case 'n':
                case 'N':
                    selected_email = (selected_email + 1) % email_count;
                    if (selected_email < email_count) {
                        emails[selected_email].read = 1;
                    }
                    break;
                case 'r':
                case 'R':
                    if (selected_email < email_count) {
                        strncpy(compose_to, emails[selected_email].sender, sizeof(compose_to) - 1);
                        snprintf(compose_subject, sizeof(compose_subject), "Re: %s", emails[selected_email].subject);
                        compose_mode = 1;
                        compose_field = 2;
                    }
                    break;
                case 'd':
                case 'D':
                    if (selected_email < email_count && email_count > 1) {
                        for (int i = selected_email; i < email_count - 1; i++) {
                            emails[i] = emails[i + 1];
                        }
                        email_count--;
                        if (selected_email >= email_count) {
                            selected_email = email_count - 1;
                        }
                    }
                    break;
                case 'f':
                case 'F':
                    if (selected_email < email_count) {
                        emails[selected_email].flagged = !emails[selected_email].flagged;
                    }
                    break;
                case 's':
                case 'S':
                    if (selected_email < email_count) {
                        emails[selected_email].starred = !emails[selected_email].starred;
                    }
                    break;
                case 'u':
                case 'U':
                    if (selected_email < email_count) {
                        emails[selected_email].read = !emails[selected_email].read;
                    }
                    break;
                case KEY_UP:
                    if (selected_email > 0) selected_email--;
                    break;
                case KEY_DOWN:
                    if (selected_email < email_count - 1) selected_email++;
                    break;
                case KEY_LEFT:
                    if (current_folder > 0) {
                        current_folder--;
                        selected_email = 0;
                    }
                    break;
                case KEY_RIGHT:
                    if (current_folder < folder_count - 1) {
                        current_folder++;
                        selected_email = 0;
                    }
                    break;
                case 'q':
                case 'Q':
                    running = 0;
                    break;
                case 'j':
                    if (selected_email < email_count - 1) selected_email++;
                    break;
                case 'k':
                    if (selected_email > 0) selected_email--;
                    break;
                case 'g':
                    current_folder = 0;
                    selected_email = 0;
                    break;
                case '$':
                    current_folder = folder_count - 1;
                    selected_email = 0;
                    break;
            }
        }
    }
    
    delwin(title_win);
    delwin(folder_win);
    delwin(list_win);
    delwin(preview_win);
    delwin(status_win);
    endwin();
    
    printf("broMail session ended.\n");
    printf("Features: Folder navigation, Email viewing/replying, Compose new emails,\n");
    printf("          Flag/Star/Unread, Delete emails, Keyboard navigation (j/k/g/$)\n");
    
    return 0;
}

void start_thunderbird_email() {
    printf("Starting Thunderbird-based email suite...\n");
}
