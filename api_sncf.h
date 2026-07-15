#pragma once

#include <pthread.h>
#include <jansson.h>
#include <stddef.h>
#include <stdbool.h>

#define SNCF_STOP_AREA_URL                              \
    "https://api.sncf.com/v1/coverage/sncf/stop_areas/" \
    "stop_area:SNCF:%s/"                          \
    "departures"

typedef struct _train_time {
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int seconds;
} train_time_t;

typedef struct _sncf_departure {
    char train_number[16];
    char line[32];
    char dest[64];
    train_time_t dep_time;
    int delay; // delay in minutes
} sncf_departure;

typedef struct _sncf_departure_table {
    char station_name[96];

    size_t n_departures;
    sncf_departure* departures;
} sncf_departure_table;

// When the departure table has just been updated,
// set the bool to true to inform the render thread
extern sncf_departure_table g_departure_table;
extern pthread_mutex_t g_departure_table_mutex;
extern bool g_departure_table_updated;
extern char* g_current_stop_area;
extern char* g_current_departures_url;

json_t* load_json_from_file(char* path);

void sncf_init_api(char *stop_area);

void sncf_get_departure_table(char* stop_area, sncf_departure_table* dep_table);

void sncf_parse_departure_table_from_json(json_t* j_root, sncf_departure_table *dep_table);

void sncf_update_departures();

void sncf_update_station_name(const char *name);
