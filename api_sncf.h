#pragma once

#include <jansson.h>
#include <stddef.h>

#define SNCF_STOP_AREA_URL                              \
    "https://api.sncf.com/v1/coverage/sncf/stop_areas/" \
    "stop_area:SNCF:87113001/"                          \
    "departures"

typedef struct _train_time {
    int hour;
    int minute;
} train_time_t;

typedef struct _sncf_departure {
    char train_number[16];
    char line[32];
    char dest[64];
    train_time_t dep_time;
    train_time_t delay;
} sncf_departure;

typedef struct _sncf_departure_table {
    char station_name[96];

    size_t n_departures;
    sncf_departure* departures;
} sncf_departure_table;

json_t* load_json_from_file(char* path);

sncf_departure_table parse_sncf_departures(char* stop_area);
