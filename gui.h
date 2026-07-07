#pragma once

#include "api_sncf.h"

void gui_init();

char gui_process_event();

void gui_terminate();

void gui_display_departures(sncf_departure_table *departures);

char *gui_station_name_entry();
