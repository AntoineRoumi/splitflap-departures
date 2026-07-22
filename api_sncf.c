#include "api_sncf.h"

#include <curl/curl.h>
#include <jansson.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "utils.h"

sncf_departure_table g_departure_table = {.n_departures = 0,
                                          .departures = NULL};
pthread_mutex_t g_departure_table_mutex = PTHREAD_MUTEX_INITIALIZER;
bool g_departure_table_updated = false;
sncf_station g_current_station = {0};
char g_current_departures_url[1024] = {0};

void sncf_set_station(sncf_station* station) {
    pthread_mutex_lock(&g_departure_table_mutex);

    strcpy(g_current_station.id, station->id);
    strcpy(g_current_station.name, station->name);
    g_departure_table_updated = true;

    if (strlen(station->id) != 0) {
        sprintf(g_current_departures_url, SNCF_DEPARTURES_STOP_AREA_URL,
                station->id);
    } else {
        g_current_departures_url[0] = '\0';
    }

    pthread_mutex_unlock(&g_departure_table_mutex);
}

// SNCF time format is yyyymmddThhmmss (T is the separator character)
// Returns { -1, -1 } in case of an error
void sncf_parse_time(char* time_text, train_time_t* time) {
    // Check if the string looks like a valid sncf time string
    if (strlen(time_text) != 15 || time_text[8] != 'T') {
        fprintf(stderr, "error: not an sncf time format\n");

        time->hour = -1;
        time->minute = -1;
        time->seconds = -1;
        return;
    }

    int result =
        sscanf(time_text, "%4d%2d%2dT%2d%2d%2d", &time->day, &time->minute,
               &time->year, &time->hour, &time->minute, &time->seconds);
    if (result != 6) {
        fprintf(stderr, "error: invalid time format\n");

        time->hour = -1;
        time->minute = -1;
        time->seconds = -1;
        return;
    }
}

// We assume that the delay is < 24h
int sncf_compute_delay(train_time_t* base_departure_time,
                       train_time_t* current_departure_time) {
    if (base_departure_time->hour > current_departure_time->hour) {
        return (
            24 * 60 -
            ((base_departure_time->hour * 60 + base_departure_time->minute)) -
            (current_departure_time->hour * 60 +
             current_departure_time->minute));
    } else {
        return 60 * (current_departure_time->hour - base_departure_time->hour) +
               (current_departure_time->minute - base_departure_time->minute);
    }
}

int sncf_autocomplete_stop_area(char* text, int max_results,
                                sncf_station* stop_areas[]) {
    static char autocomplete_url[1024];
    sprintf(autocomplete_url, SNCF_AUTOCOMPLETE_STOP_AREA_URL, text);

    json_t* j_root = load_json_from_url(autocomplete_url);

    if (!j_root) {
        fprintf(stderr, "error: couldn't load autocomplete json\n");
        return 0;
    }

    if (!json_is_object(j_root)) {
        fprintf(stderr, "error: root is not an object\n");
        json_decref(j_root);
        return 0;
    }

    json_t* j_results = json_object_get(j_root, "pt_objects");

    // No result
    if (!j_results) {
        json_decref(j_root);
        return 0;
    }
    if (!json_is_array(j_results)) {
        fprintf(stderr, "error: results is not an array\n");
        json_decref(j_root);
        return 0;
    }

    int stop_areas_count = json_array_size(j_results);
    if (stop_areas_count > max_results) {
        stop_areas_count = max_results;
    }

    json_t* j_result;
    char *id, *name;
    sncf_station* current_stop_area;
    for (int i = 0; i < stop_areas_count; i++) {
        j_result = json_array_get(j_results, i);
        current_stop_area = &(*stop_areas)[i];
        json_unpack(j_result, "{s:s,s:s}", "id", &id, "name", &name);

        strncpy(current_stop_area->id, id, 63);
        strncpy(current_stop_area->name, name, 63);
    }

    json_decref(j_root);

    return stop_areas_count;
}

void sncf_parse_departure_table_from_json(json_t* j_root,
                                          sncf_departure_table* dep_table) {
    if (!j_root) {
        fprintf(stderr, "error: couldn't load departure table json\n");
        return;
    }

    if (!json_is_object(j_root)) {
        fprintf(stderr, "error: root is not an object\n");
        json_decref(j_root);
        return;
    }

    json_t* j_departures = json_object_get(j_root, "departures");
    if (!j_departures) {
        fprintf(stderr, "error: departures is null\n");
        json_decref(j_root);
        return;
    }
    if (!json_is_array(j_departures)) {
        fprintf(stderr, "error: departures is not an array\n");
        json_decref(j_root);
        return;
    }

    dep_table->n_departures = json_array_size(j_departures);
    dep_table->departures =
        realloc(dep_table->departures,
                dep_table->n_departures * sizeof(sncf_departure));
    if (dep_table->departures == NULL) {
        fprintf(stderr,
                "error: couldn't allocate departure_table.departures\n");
        dep_table->n_departures = 0;
        json_decref(j_root);
    }

    json_t* j_dep;
    char *train_number, *commercial_mode, *code, *dest, *base_departure_time,
        *current_departure_time;
    sncf_departure* dep;
    for (size_t i = 0; i < json_array_size(j_departures); ++i) {
        j_dep = json_array_get(j_departures, i);
        if (!json_is_object(j_dep)) {
            fprintf(stderr, "error: departure is not an object\n");
            json_decref(j_root);
        } else {
            dep = &dep_table->departures[i];
            json_unpack(j_dep, "{s: {s:s, s:s}, s: {s:s, s:s, s:s, s:s}}",
                        "stop_date_time", "base_departure_date_time",
                        &base_departure_time, "departure_date_time",
                        &current_departure_time, "display_informations",
                        "direction", &dest, "commercial_mode", &commercial_mode,
                        "code", &code, "headsign", &train_number);

            sscanf(dest, "%[^(]", dep_table->departures[i].dest);
            trim_right(dep->dest);
            if (strlen(code) == 0) {
                strcpy(dep->line, commercial_mode);
            } else {
                snprintf(dep->line, 32, "%s %s", commercial_mode, code);
            }
            strcpy(dep->train_number, train_number);

            train_time_t base_datetime, current_datetime;
            sncf_parse_time(base_departure_time, &base_datetime);
            sncf_parse_time(current_departure_time, &current_datetime);

            dep->delay = sncf_compute_delay(&base_datetime, &current_datetime);
            dep->dep_time = current_datetime;
        }
    }
}

void sncf_update_departures() {
    if (g_current_departures_url[0] == '\0') {
        return;
    }

    json_t* j_content = load_json_from_url(g_current_departures_url);

    pthread_mutex_lock(&g_departure_table_mutex);

    sncf_parse_departure_table_from_json(j_content, &g_departure_table);

    json_decref(j_content);

    g_departure_table_updated = true;

    pthread_mutex_unlock(&g_departure_table_mutex);
}
