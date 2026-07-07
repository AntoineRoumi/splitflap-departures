#include "./api.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

void api_init() {
    CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
    if (result != CURLE_OK) {
        fprintf(stderr, "error: cannot initialize libcurl (error code %d)\n",
                (int)result);
    }
}

void api_cleanup() { curl_global_cleanup(); }

response_t api_response_init(size_t buffer_size) {
    response_t response = {.buffer_size = 0, .pos = 0, .content = NULL};

    response.content = malloc(buffer_size);
    if (response.content == NULL) {
        fprintf(stderr, "error: cannot alloc response content buffer\n");
        return response;
    }

    response.buffer_size = buffer_size;

    return response;
}

void api_response_clean(response_t* response) {
    response->buffer_size = 0;
    response->pos = 0;
    free(response->content);
}

static size_t api_write_file(char* buffer, size_t size, size_t nmemb,
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

void api_set_auth_headers(struct curl_slist **headers_list) {
    char *auth_header = malloc(strlen(g_config.sncf_auth_key) + strlen("Authorization: ") + 1);
    sprintf(auth_header, "Authorization: %s", g_config.sncf_auth_key);
    *headers_list = curl_slist_append(*headers_list, auth_header);
}

response_t api_get(char* url) {
    response_t response = {.content = NULL, .buffer_size = 0, .pos = 0};

    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "error: cannot initialize easy curl\n");
        return response;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    struct curl_slist* list = NULL;
    // TODO remove API key in the future

    api_set_auth_headers(&list);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    response = api_response_init(BUFFER_SIZE);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "error: couldn't perform curl request %s\n",
                curl_easy_strerror(result));
        api_response_clean(&response);
        return response;
    }

    curl_easy_cleanup(curl);

    return response;
}
