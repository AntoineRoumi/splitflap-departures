#include <bits/types/mbstate_t.h>
#include <curses.h>
#include <ncursesw/curses.h>
#include <pthread.h>
#include <string.h>
#include <wchar.h>

#include "api_sncf.h"
#include "config.h"
#include "gui.h"
#include "net.h"
#include "update.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    // Load config
    load_config();

    // Init curl
    net_init();

    sncf_station selected_station = {.id = "stop_area:SNCF:87686006",
                                     .name = "Gare de Lyon"};
    sncf_init_api(&selected_station);

    gui_init();

    // Update every minute
    int update_time_sec = 60;
    update_t* departure_table_updater;
    departure_table_updater =
        update_start(update_time_sec, sncf_update_departures);

    char c = ' ';
    for (;;) {
        c = gui_process_event();

        switch (c) {
            case 's':
                gui_station_name_entry(&selected_station);

                sncf_set_station(&selected_station);

                update_restart(departure_table_updater);
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
