#pragma once

#include <wchar.h>

#define CLAMP(x, min, max) \
    (((x) > (max)) ? (max) : (((x) < (min)) ? (min) : (x)))

void trim_right(char* text);

int utf8_chrlen(const char* s, int pos);
int utf8_strlen(const char* s);
int utf8_string_to_wstring(char* text, wchar_t* wtext, int max_bytes);
