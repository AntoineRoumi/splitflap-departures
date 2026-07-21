#include <bits/types/mbstate_t.h>
#include <curses.h>
#include <ncursesw/curses.h>
#include <pthread.h>

#include "api_sncf.h"
#include "audio.h"
#include "config.h"
#include "gui.h"
#include "net.h"
#include "update.h"

int main(int argc, char* argv[]) {
    // Load config
    load_config();

    // Init curl
    net_init();

    sncf_station selected_station = {0};
    sncf_set_station(&selected_station);

    gui_init();

    audio_init();

    // Update every minute
    update_t departure_table_updater;
    update_create(&departure_table_updater, g_config.update_interval,
                  sncf_update_departures);
    update_start(&departure_table_updater);

    int c;
    for (;;) {
        while ((c = gui_process_event()) != ERR) {
            switch (c) {
                case 's':
                    gui_station_name_entry(&selected_station);

                    sncf_set_station(&selected_station);

                    update_restart(&departure_table_updater);
                    break;
                case 'q':
                    goto _exit;
                    break;
            }
        }

        gui_update_display();

        gui_sleep();
    }

_exit:
    update_stop(&departure_table_updater);

    audio_cleanup();

    gui_terminate();

    // Cleanup curl
    net_cleanup();

    return 0;
}
