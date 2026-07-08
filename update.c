#include "update.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "net.h"

static void* update_net_thread(void* ptr) {
    update_net_thread_input* input = ptr;

    response_t response;
    while (!(*input->stop_update)) {
        response = net_get(input->url);

        input->callback(&response);
        printf("-> %d\n", input->interval_seconds);

        net_response_clean(&response);

        sleep(input->interval_seconds);
    }

    return NULL;
}

update_t* update_net_start(char* url, int interval_seconds,
                           update_net_callback callback) {
    update_t* update_object = malloc(sizeof(update_t));
    update_object->input.stop_update = false;

    update_object->input.url = "Hello world\n";
    update_object->input.interval_seconds = interval_seconds;
    update_object->input.callback = callback;
    update_object->input.stop_update = &update_object->stop_update;

    int ret = pthread_create(&update_object->thread_id, NULL, update_net_thread,
                             (void*)&update_object->input);
    if (ret) {
        fprintf(stderr, "error: cannot create update thread\n");
        exit(EXIT_FAILURE);
    }

    return update_object;
}

void update_net_stop(update_t* update) {
    update->stop_update = true;

    pthread_join(update->thread_id, NULL);

    free(update);
}
