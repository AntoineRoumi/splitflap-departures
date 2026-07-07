#pragma once

typedef struct config_ {
    char *sncf_auth_key;
} config;

extern config g_config;

// Load .env file (no sanitization)
void load_config();
