#include <ncurses.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "net.h"
#include "api_sncf.h"
#include "config.h"
#include "gui.h"

int main(int argc, char* argv[]) {
    // Load config
    load_config();

    // Init curl
    net_init();

    sncf_departure_table dep_table = sncf_parse_departures("");

    gui_init();

    gui_display_departures(&dep_table);

    char c = ' ';
    char* station_name;
    for (;;) {
        c = gui_process_event();

        switch (c) {
            case 's':
                station_name = gui_station_name_entry();
                strcpy(dep_table.station_name, station_name);
                free(station_name);
                break;
            case 'q':
                goto _exit;
                break;
        }

        gui_display_departures(&dep_table);
    }

_exit:

    gui_terminate();

    // Cleanup curl
    net_cleanup();

    return 0;
}
