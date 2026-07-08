#include "json.h"

#include <stdio.h>

#include "net.h"

json_t* load_json_from_file(char* path) {
    json_t* j_root;
    json_error_t error;

    j_root = json_load_file(path, 0, &error);

    if (!j_root) {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return NULL;
    }

    return j_root;
}

json_t* load_json_from_url(char* url) {
    json_t* j_root;
    json_error_t error;

    response_t response = net_get(url);
    if (response.content == NULL) {
        fprintf(stderr, "error: no content in response from %s", url);
        return NULL;
    }

    j_root = json_loads(response.content, 0, &error);

    if (!j_root) {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return NULL;
    }

    net_response_clean(&response);

    return j_root;
}
