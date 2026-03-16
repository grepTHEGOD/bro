#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>
#include <ctype.h>
#include <stdbool.h>

extern int has_prefix(const char *str, const char *prefix);

#define BUFFER_SIZE 65536

typedef struct {
    int id;
    char method[64];
    json_object *params;
} lsp_request;

typedef struct {
    json_object *result;
    json_object *error;
} lsp_response;

static int next_id = 1;
static char root_path[512] = ".";

void send_response(int id, json_object *result) {
    json_object *response = json_object_new_object();
    json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(response, "id", json_object_new_int(id));
    json_object_object_add(response, "result", result ? result : json_object_new_object());
    
    const char *str = json_object_to_json_string_ext(response, JSON_C_TO_STRING_PRETTY);
    printf("Content-Length: %zu\r\n\r\n%s", strlen(str), str);
    fflush(stdout);
    
    json_object_put(response);
}

void send_notification(const char *method, json_object *params) {
    json_object *notification = json_object_new_object();
    json_object_object_add(notification, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(notification, "method", json_object_new_string(method));
    if (params) {
        json_object_object_add(notification, "params", params);
    }
    
    const char *str = json_object_to_json_string_ext(notification, JSON_C_TO_STRING_PRETTY);
    printf("Content-Length: %zu\r\n\r\n%s", strlen(str), str);
    fflush(stdout);
    
    json_object_put(notification);
}

int read_content(char *buffer, size_t max_size) {
    char header[256];
    
    size_t total = 0;
    while (total < sizeof(header) - 1) {
        char c;
        if (fread(&c, 1, 1, stdin) != 1) return -1;
        if (c == '\r') continue;
        if (c == '\n' && total > 0 && header[total-1] == '\r') {
            header[total-1] = '\0';
            break;
        }
        header[total++] = c;
    }
    
    if (!has_prefix(header, "Content-Length:")) {
        return -1;
    }
    
    int content_length = atoi(header + 15);
    if (content_length >= max_size) {
        return -1;
    }
    
    if (fread(buffer, 1, content_length, stdin) != content_length) {
        return -1;
    }
    buffer[content_length] = '\0';
    
    return content_length;
}

json_object *parse_position(json_object *params) {
    json_object *position_obj;
    if (json_object_object_get_ex(params, "position", &position_obj)) {
        return position_obj;
    }
    json_object *text_doc;
    if (json_object_object_get_ex(params, "textDocument", &text_doc)) {
        json_object_object_get_ex(text_doc, "position", &position_obj);
        return position_obj;
    }
    return NULL;
}

json_object *parse_text_document_uri(json_object *params) {
    json_object *text_doc;
    if (!json_object_object_get_ex(params, "textDocument", &text_doc)) {
        return NULL;
    }
    
    json_object *uri;
    if (!json_object_object_get_ex(text_doc, "uri", &uri)) {
        return NULL;
    }
    
    return uri;
}

const char *uri_to_path(const char *uri) {
    static char path[512];
    
    if (has_prefix(uri, "file://")) {
        const char *path_start = uri + 7;
        strcpy(path, path_start);
        
        char *escaped = strchr(path, '%');
        while (escaped) {
            char hex[3] = {escaped[1], escaped[2], '\0'};
            int chr;
            sscanf(hex, "%02x", &chr);
            *escaped = chr;
            memmove(escaped + 1, escaped + 3, strlen(escaped + 3) + 1);
            escaped = strchr(path, '%');
        }
    } else {
        strcpy(path, uri);
    }
    
    return path;
}

void handle_initialize(json_object *params) {
    json_object *root_uri;
    if (json_object_object_get_ex(params, "rootUri", &root_uri)) {
        const char *uri = json_object_get_string(root_uri);
        strcpy(root_path, uri_to_path(uri));
    }
    
    json_object *result = json_object_new_object();
    json_object_object_add(result, "capabilities", json_object_new_object());
    
    json_object *caps;
    json_object_object_get_ex(result, "capabilities", &caps);
    json_object_object_add(caps, "textDocumentSync", json_object_new_int(1));
    json_object_object_add(caps, "definitionProvider", json_object_new_boolean(1));
    json_object_object_add(caps, "referencesProvider", json_object_new_boolean(1));
    json_object_object_add(caps, "documentHighlightProvider", json_object_new_boolean(1));
    json_object_object_add(caps, "hoverProvider", json_object_new_boolean(1));
    json_object_object_add(caps, "completionProvider", json_object_new_object());
    
    send_response(next_id++, result);
}

void handle_initialized(json_object *params) {
    printf("bro LSP: Server initialized for %s\n", root_path);
    fflush(stdout);
}

void handle_shutdown(json_object *params) {
    send_response(next_id++, json_object_new_object());
}

void handle_text_document_did_open(json_object *params) {
    json_object *text_doc;
    if (!json_object_object_get_ex(params, "textDocument", &text_doc)) return;
    
    json_object *uri, *text, *version;
    if (!json_object_object_get_ex(text_doc, "uri", &uri)) return;
    if (!json_object_object_get_ex(text_doc, "text", &text)) return;
    if (!json_object_object_get_ex(text_doc, "version", &version)) return;
    
    printf("bro LSP: Opened %s (v%d)\n", 
           json_object_get_string(uri), 
           json_object_get_int(version));
    fflush(stdout);
}

void handle_text_document_did_change(json_object *params) {
    json_object *text_doc;
    if (!json_object_object_get_ex(params, "textDocument", &text_doc)) return;
    
    json_object *uri, *version;
    if (!json_object_object_get_ex(text_doc, "uri", &uri)) return;
    if (!json_object_object_get_ex(text_doc, "version", &version)) return;
    
    printf("bro LSP: Changed %s (v%d)\n", 
           json_object_get_string(uri), 
           json_object_get_int(version));
    fflush(stdout);
}

void handle_text_document_definition(json_object *params) {
    json_object *uri = parse_text_document_uri(params);
    json_object *pos = parse_position(params);
    
    if (!uri || !pos) {
        send_response(next_id++, NULL);
        return;
    }
    
    json_object *result = json_object_new_array();
    
    json_object *location = json_object_new_object();
    json_object_object_add(location, "uri", json_object_new_string(json_object_get_string(uri)));
    
    json_object *range = json_object_new_object();
    json_object_object_add(range, "start", json_object_get(pos));
    json_object_object_add(range, "end", json_object_get(pos));
    json_object_object_add(location, "range", range);
    
    json_object_array_add(result, location);
    
    send_response(next_id++, result);
}

void handle_text_document_hover(json_object *params) {
    json_object *uri = parse_text_document_uri(params);
    json_object *pos = parse_position(params);
    
    if (!uri || !pos) {
        send_response(next_id++, NULL);
        return;
    }
    
    const char *path = uri_to_path(json_object_get_string(uri));
    
    FILE *f = fopen(path, "r");
    if (!f) {
        send_response(next_id++, NULL);
        return;
    }
    
    int line = json_object_get_int(json_object_object_get(pos, "line"));
    int char_pos = json_object_get_int(json_object_object_get(pos, "character"));
    
    char buffer[1024];
    int current_line = 0;
    char *word = NULL;
    
    while (fgets(buffer, sizeof(buffer), f)) {
        if (current_line == line) {
            int len = strlen(buffer);
            if (char_pos < len) {
                int word_start = char_pos;
                while (word_start > 0 && (isalnum(buffer[word_start-1]) || buffer[word_start-1] == '_')) {
                    word_start--;
                }
                int word_end = char_pos;
                while (word_end < len && (isalnum(buffer[word_end]) || buffer[word_end] == '_')) {
                    word_end++;
                }
                if (word_end > word_start) {
                    buffer[word_end] = '\0';
                    word = strdup(buffer + word_start);
                }
            }
            break;
        }
        current_line++;
    }
    fclose(f);
    
    json_object *result = json_object_new_object();
    json_object *contents = json_object_new_object();
    json_object_object_add(contents, "kind", json_object_new_string("markdown"));
    
    char hover_text[256];
    if (word) {
        snprintf(hover_text, sizeof(hover_text), "**%s**\n\nType: string\nDefined in: %s:%d", word, path, line + 1);
        free(word);
    } else {
        snprintf(hover_text, sizeof(hover_text), "No info available\nFile: %s:%d", path, line + 1);
    }
    
    json_object_object_add(contents, "value", json_object_new_string(hover_text));
    json_object_object_add(result, "contents", contents);
    
    send_response(next_id++, result);
}

void handle_text_document_completion(json_object *params) {
    json_object *uri = parse_text_document_uri(params);
    json_object *pos = parse_position(params);
    
    json_object *result = json_object_new_array();
    
    const char *keywords[] = {"if", "else", "while", "for", "return", "int", "char", "void", "struct", "typedef"};
    for (int i = 0; i < 10; i++) {
        json_object *item = json_object_new_object();
        json_object_object_add(item, "label", json_object_new_string(keywords[i]));
        json_object_object_add(item, "kind", json_object_new_int(14));
        json_object_array_add(result, item);
    }
    
    send_response(next_id++, result);
}

void handle_text_document_references(json_object *params) {
    send_response(next_id++, json_object_new_array());
}

void handle_text_document_document_highlight(json_object *params) {
    send_response(next_id++, json_object_new_array());
}

void handle_exit(json_object *params) {
    exit(0);
}

void process_request(json_object *request) {
    json_object *id;
    json_object *method;
    json_object *params = NULL;
    
    if (!json_object_object_get_ex(request, "id", &id)) {
        return;
    }
    
    if (!json_object_object_get_ex(request, "method", &method)) {
        return;
    }
    
    json_object_object_get_ex(request, "params", &params);
    
    const char *method_str = json_object_get_string(method);
    int req_id = json_object_get_int(id);
    
    if (strcmp(method_str, "initialize") == 0) {
        handle_initialize(params);
    } else if (strcmp(method_str, "initialized") == 0) {
        handle_initialized(params);
    } else if (strcmp(method_str, "shutdown") == 0) {
        handle_shutdown(params);
    } else if (strcmp(method_str, "exit") == 0) {
        handle_exit(params);
    } else if (strcmp(method_str, "textDocument/didOpen") == 0) {
        handle_text_document_did_open(params);
    } else if (strcmp(method_str, "textDocument/didChange") == 0) {
        handle_text_document_did_change(params);
    } else if (strcmp(method_str, "textDocument/definition") == 0) {
        handle_text_document_definition(params);
    } else if (strcmp(method_str, "textDocument/hover") == 0) {
        handle_text_document_hover(params);
    } else if (strcmp(method_str, "textDocument/completion") == 0) {
        handle_text_document_completion(params);
    } else if (strcmp(method_str, "textDocument/references") == 0) {
        handle_text_document_references(params);
    } else if (strcmp(method_str, "textDocument/documentHighlight") == 0) {
        handle_text_document_document_highlight(params);
    } else {
        printf("bro LSP: Unknown method: %s\n", method_str);
        fflush(stdout);
    }
}

int cmd_live(int argc, char **argv) {
    printf("bro v1.0.0 - Language Server Protocol\n");
    printf("=====================================\n");
    printf("Starting LSP server on stdin/stdout...\n");
    printf("Connect with your IDE (VS Code, Neovim, etc.)\n\n");
    fflush(stdout);
    
    char buffer[BUFFER_SIZE];
    
    while (1) {
        int len = read_content(buffer, BUFFER_SIZE);
        if (len < 0) {
            break;
        }
        
        json_object *request = json_tokener_parse(buffer);
        if (!request) {
            continue;
        }
        
        process_request(request);
        json_object_put(request);
    }
    
    return 0;
}
