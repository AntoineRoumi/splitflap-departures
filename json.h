#pragma once

#include <jansson.h>

#include "api.h"

json_t *load_json_from_file(char *path);

json_t *load_json_from_url(char *url);
