#include "config.h"

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

config g_config = {.sncf_auth_key = NULL};

char* load_config_string(json_t* j_root, const char* key) {
    json_t* j_value = json_object_get(j_root, key);

    if (!j_value) {
        fprintf(stderr, "error: no key '%s' in config.json\n", key);
        exit(EXIT_FAILURE);
    }
    if (!json_is_string(j_value)) {
        fprintf(stderr, "error: %s is not a string in config.json\n", key);
        exit(EXIT_FAILURE);
    }

    const char* value = json_string_value(j_value);

    if (value == NULL) {
        fprintf(stderr, "error: value of %s is invalid\n", key);
        exit(EXIT_FAILURE);
    }

    // Copy the value into a new variable
    char* config_value = malloc(strlen(value) + 1);
    strcpy(config_value, value);

    return config_value;
}

void load_config() {
    json_t* j_root;
    json_error_t error;

    j_root = json_load_file("config.json", 0, &error);

    if (!j_root) {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        exit(EXIT_FAILURE);
    }
    if (!json_is_object(j_root)) {
        fprintf(stderr, "error: config.json format is invalid\n");
        exit(EXIT_FAILURE);
    }

    g_config.sncf_auth_key = load_config_string(j_root, "sncf_auth_key");
}
