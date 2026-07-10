#include <ncurses.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "api_sncf.h"
#include "config.h"
#include "gui.h"
#include "net.h"
#include "update.h"

int main(int argc, char* argv[]) {
    // Load config
    load_config();

    // Init curl
    net_init();

    gui_init();

    // Update every minute
    update_t *departure_table_updater = update_start(60, sncf_update_departures);

    char c = ' ';
    char* station_name;
    for (;;) {
        c = gui_process_event();

        switch (c) {
            case 's':
                station_name = gui_station_name_entry();

                sncf_update_station_name(station_name);

                free(station_name);
                break;
            case 'q':
                goto _exit;
                break;
        }

        if (g_departure_table_updated) {
            gui_update_departures();
        }
    }

_exit:
    update_stop(departure_table_updater);

    gui_terminate();

    // Cleanup curl
    net_cleanup();

    return 0;
}
