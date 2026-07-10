#pragma once

#include <pthread.h>
#include <stdbool.h>

typedef void (*update_callback_t)(void);

typedef struct _update_thread_input {
    int interval_seconds;
    update_callback_t callback;
} update_thread_input_t;

typedef struct _update {
    pthread_t thread_id;
    update_thread_input_t input;
} update_t;

update_t* update_start(int interval_seconds,
                           update_callback_t callback);

void update_stop(update_t* update);
