#include "net.h"

#include <curl/curl.h>
#include <curl/typecheck-gcc.h>
#include <ncursesw/curses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

CURL* net_curl;

void net_init() {
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    if (result != CURLE_OK) {
        fprintf(stderr, "error: cannot initialize libcurl (error code %d)\n",
                (int)result);
    }

    net_curl = curl_easy_init();
    if (!net_curl) {
        fprintf(stderr, "error: cannot initialize easy curl\n");
        exit(EXIT_FAILURE);
    }
}

void net_cleanup() {
    curl_easy_cleanup(net_curl);
    curl_global_cleanup();
}

response_t net_response_init(size_t buffer_size) {
    response_t response = {.buffer_size = 0, .pos = 0, .content = NULL};

    response.content = malloc(buffer_size);
    if (response.content == NULL) {
        fprintf(stderr, "error: cannot alloc response content buffer\n");
        return response;
    }

    response.buffer_size = buffer_size;

    return response;
}

void net_response_clean(response_t* response) {
    response->buffer_size = 0;
    response->pos = 0;
    free(response->content);
}

static size_t net_write_buffer(char* buffer, size_t size, size_t nmemb,
                               void* stream) {
    response_t* response = (response_t*)stream;

    if (response->pos + size * nmemb >= response->buffer_size) {
        fprintf(stderr, "error: buffer is too small\n");
        return 0;
    }

    memcpy(response->content + response->pos, buffer, size * nmemb);
    response->pos += size * nmemb;

    return size * nmemb;
}

void net_set_auth_headers(struct curl_slist** headers_list) {
    char* auth_header =
        malloc(strlen(g_config.sncf_auth_key) + strlen("Authorization: ") + 1);
    sprintf(auth_header, "Authorization: %s", g_config.sncf_auth_key);
    *headers_list = curl_slist_append(*headers_list, auth_header);
}

char* net_get(char* url) {
    if (!net_curl) {
        fprintf(stderr, "error: libcurl is uninitialized\n");
    }

    CURLUcode ret;
    CURLU* encoded_url = curl_url();
    ret = curl_url_set(encoded_url, CURLUPART_URL, url,
                       CURLU_URLENCODE | CURLU_ALLOW_SPACE);
    if (ret) {
        fprintf(stderr, "error: couldn't set curl url for url %s", url);
        return NULL;
    }

    curl_easy_setopt(net_curl, CURLOPT_CURLU, encoded_url);

    struct curl_slist* list = NULL;

    net_set_auth_headers(&list);

    curl_easy_setopt(net_curl, CURLOPT_HTTPHEADER, list);

    response_t response = net_response_init(BUFFER_SIZE);

    curl_easy_setopt(net_curl, CURLOPT_WRITEFUNCTION, net_write_buffer);
    curl_easy_setopt(net_curl, CURLOPT_WRITEDATA, &response);

    CURLcode result = curl_easy_perform(net_curl);

    if (result != CURLE_OK) {
        fprintf(stderr, "error: couldn't perform curl request %s\n",
                curl_easy_strerror(result));
        net_response_clean(&response);
        return NULL;
    }

    long response_code;
    curl_easy_getinfo(net_curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code != 200) {
        return NULL;
    }

    response.content[response.pos++] = '\0';  // NULL terminate the string

    return response.content;
}
