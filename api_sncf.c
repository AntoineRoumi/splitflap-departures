#include "api_sncf.h"

#include <curl/curl.h>
#include <jansson.h>
#include <stdio.h>
#include <string.h>

#include "json.h"
#include "utils.h"


// SNCF time format is yyyymmddThhmmss (T is the separator character)
// Returns { -1, -1 } in case of an error
train_time_t sncf_parse_time(char* time_text) {
    train_time_t time = {.hour = -1, .minute = -1};

    // Check if the string looks like a valid sncf time string
    if (strlen(time_text) != 15 || time_text[8] != 'T') {
        fprintf(stderr, "error: not an sncf time format\n");
        return time;
    }

    int result = sscanf(time_text + 9, "%2d%2d", &time.hour, &time.minute);
    if (result != 2) {
        fprintf(stderr, "error: invalid time format\n");
        time.hour = -1;
        time.minute = -1;
        return time;
    }

    return time;
}

sncf_departure_table sncf_parse_departures(char* stop_area) {
    sncf_departure_table dep_table = {.n_departures = 0, .departures = NULL};

    json_t* j_root = load_json_from_url(SNCF_STOP_AREA_URL);
    if (!j_root) {
        fprintf(stderr, "error: couldn't load json from %s\n",
                SNCF_STOP_AREA_URL);
        return dep_table;
    }

    if (!json_is_object(j_root)) {
        fprintf(stderr, "error: root is not an object\n");
        json_decref(j_root);
        return dep_table;
    }

    json_t* j_departures = json_object_get(j_root, "departures");
    if (!j_departures) {
        fprintf(stderr, "error: departures is null\n");
        json_decref(j_root);
        return dep_table;
    }
    if (!json_is_array(j_departures)) {
        fprintf(stderr, "error: departures is not an array\n");
        json_decref(j_root);
        return dep_table;
    }

    dep_table.n_departures = json_array_size(j_departures);
    dep_table.departures =
        malloc(dep_table.n_departures * sizeof(sncf_departure));
    if (dep_table.departures == NULL) {
        fprintf(stderr,
                "error: couldn't allocate departure_table.departures\n");
        dep_table.n_departures = 0;
        json_decref(j_root);
        return dep_table;
    }

    json_t* j_dep;
    char *train_number, *commercial_mode, *code, *dest, *datetime;
    sncf_departure* dep;
    for (size_t i = 0; i < json_array_size(j_departures); ++i) {
        j_dep = json_array_get(j_departures, i);
        if (!json_is_object(j_dep)) {
            fprintf(stderr, "error: departure is not an object\n");
            json_decref(j_root);
        } else {
            dep = &dep_table.departures[i];
            json_unpack(j_dep, "{s: {s: s}, s: {s:s, s:s, s:s, s:s}}",
                        "stop_date_time", "departure_date_time", &datetime,
                        "display_informations", "direction", &dest,
                        "commercial_mode", &commercial_mode, "code", &code,
                        "headsign", &train_number);

            sscanf(dest, "%[^(]", dep_table.departures[i].dest);
            trim_right(dep->dest);
            if (strlen(code) == 0) {
                strcpy(dep->line, commercial_mode);
            } else {
                snprintf(dep->line, 32, "%s %s", commercial_mode, code);
            }
            strcpy(dep->train_number, train_number);
            dep_table.departures[i].dep_time = sncf_parse_time(datetime);
        }
    }

    return dep_table;
}
