#pragma once

#include "api_sncf.h"

#define GUI_SPLITFLAP_ANIMATION_FPS 20.f

void gui_init();

char gui_process_event();

void gui_terminate();

void gui_update_departures();

void gui_render_departures(sncf_departure_table* departures);

void gui_station_name_entry(sncf_station* station);

