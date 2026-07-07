#pragma once

#include <stddef.h>

#define BUFFER_SIZE (256 * 1024)  // 256kB

typedef struct _response {
    size_t buffer_size;
    size_t pos;
    char* content;
} response_t;

response_t api_response_init(size_t content_max_size);

void api_response_clean(response_t* response);

void api_init();

void api_cleanup();

response_t api_get(char* url);
