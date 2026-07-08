#pragma once

#include <jansson.h>

json_t *load_json_from_file(char *path);

json_t *load_json_from_url(char *url);
