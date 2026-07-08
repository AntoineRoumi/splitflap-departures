#pragma once

#include <pthread.h>
#include <stdbool.h>

#include "net.h"

typedef void (*update_net_callback)(response_t*);

typedef struct _update_net_thread_input {
    char* url;
    int interval_seconds;
    update_net_callback callback;
    bool* stop_update;
} update_net_thread_input;

typedef struct _update {
    pthread_t thread_id;
    bool stop_update;
    update_net_thread_input input;
} update_t;

update_t* update_net_start(char* url, int interval_seconds,
                           update_net_callback callback);

void update_net_stop(update_t* update);
