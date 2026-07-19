#include "update.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "net.h"

static void* update_thread(void* ptr) {
    update_thread_input_t* input = ptr;

    // We sleep for chunks of 0.1 second each
    static struct timespec sleep_interval = { .tv_sec = 0, .tv_nsec = 1e8 };

    int sleep_chunks;
    while (1) {
        input->callback();

        sleep_chunks = 0;
        while (sleep_chunks < input->update_interval_s) {
            if (input->stop) {
                goto _update_end;
            }
            if (input->restart) {
                input->restart = false;
                break;
            }
            nanosleep(&sleep_interval, NULL);
            ++sleep_chunks;
        }
    }

_update_end:

    return NULL;
}

void update_create(update_t *update, int interval_seconds, update_callback_t callback) {
    update->input.update_interval_s = interval_seconds;
    update->input.callback = callback;
}

void update_start(update_t* update) {
    int ret = pthread_create(&update->thread_id, NULL, update_thread,
                             (void*)&update->input);

    if (ret) {
        fprintf(stderr, "error: cannot create update thread\n");
        exit(EXIT_FAILURE);
    }
}

void update_stop(update_t* update) {
    update->input.stop = true;
    pthread_join(update->thread_id, NULL);
}

void update_restart(update_t* update) { 
    update->input.restart = true; 
}
