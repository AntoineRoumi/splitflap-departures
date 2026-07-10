#include "update.h"

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "net.h"

static void* update_thread(void* ptr) {
    update_thread_input_t* input = ptr;

    while (1) {
        input->callback();

        sleep(input->interval_seconds);
    }

    return NULL;
}

update_t* update_start(int interval_seconds, update_callback_t callback) {
    update_t* update_object = malloc(sizeof(update_t));

    update_object->input.interval_seconds = interval_seconds;
    update_object->input.callback = callback;

    int ret = pthread_create(&update_object->thread_id, NULL, update_thread,
                             (void*)&update_object->input);
    if (ret) {
        fprintf(stderr, "error: cannot create update thread\n");
        exit(EXIT_FAILURE);
    }

    return update_object;
}

void update_stop(update_t* update) {
    pthread_kill(update->thread_id, SIGINT);

    free(update);
}
