#include "utils.h"

#include <jansson.h>

void trim_right(char* text) {
    for (int i = strlen(text) - 1; i >= 0; --i) {
        if (!isspace(text[i])) {
            text[i + 1] = '\0';
            return;
        }
    }
}
