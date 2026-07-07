#include <ncurses.h>
#include <string.h>

#include "./api_sncf.h"
#include "api.h"
#include "config.h"
#include "gui.h"

int main(int argc, char* argv[]) {
    // Load config
    load_config();

    // Init curl
    api_init();

    sncf_departure_table dep_table = parse_sncf_departures("");

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
    api_cleanup();

    return 0;
}
