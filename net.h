#pragma once

#include <stddef.h>

#define BUFFER_SIZE (256 * 1024)  // 256kB

typedef struct _response {
    size_t buffer_size;
    size_t pos;
    char* content;
} response_t;

typedef struct _request {
    char* url;
    struct curl_slist* headers;
} request_t;

response_t net_response_init(size_t content_max_size);

void net_response_clean(response_t* response);

void net_init();

void net_cleanup();

char* net_get(char* url);
