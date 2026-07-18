#include "json.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "jansson.h"
#include "net.h"

json_t* load_json_from_text(char* text) {
    json_t *j_root;
    json_error_t error;
    j_root = json_loads(text, 0, &error);

    if (!j_root) {
        printw("error: on line %d:%d: %s\n", error.line, error.column, error.text);
        return NULL;
    }

    return j_root;
}

json_t* load_json_from_file(char* path) {
    json_t* j_root;
    json_error_t error;

    j_root = json_load_file(path, 0, &error);

    if (!j_root) {
        fprintf(stderr, "error: on line %d:%d: %s\n", error.line, error.column, error.text);
        return NULL;
    }

    return j_root;
}

json_t* load_json_from_url(char* url) {
    json_t* j_root;

    char* content = net_get(url);

    if (content == NULL) {
        fprintf(stderr, "error: no content in response from %s\n", url);
        return NULL;
    }

    j_root = load_json_from_text(content);

    free(content);

    return j_root;
}
