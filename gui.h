#pragma once

#include "api_sncf.h"

typedef struct _departures_display {
    int n_lines;
    char** lines;
} departures_display;

void gui_init_departures_display(int n_lines, departures_display* display);

void gui_init();

char gui_process_event();

void gui_terminate();

void gui_update_departures();

void gui_display_departures(sncf_departure_table* departures);

void gui_generate_departures_display(sncf_departure_table* departures, departures_display *display);

void gui_splitflap_animate(departures_display *old_display, departures_display *new_display);

void gui_station_name_entry(sncf_station* station);

