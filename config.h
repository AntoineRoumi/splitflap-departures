#pragma once

typedef struct config_ {
    char *sncf_auth_key;

    int splitflap_fps;
    int update_interval;
} config;

extern config g_config;

// Load json config file
// I use json since I'm already using Jansson for web responses
void load_config();
