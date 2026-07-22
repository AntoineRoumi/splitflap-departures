#include "utils.h"

#include <ctype.h>
#include <jansson.h>
#include <string.h>
#include <wchar.h>

void trim_right(char* text) {
    for (int i = strlen(text) - 1; i >= 0; --i) {
        if (!isspace(text[i])) {
            text[i + 1] = '\0';
            return;
        }
    }
}

int utf8_is_continuation(char c) { return (c & 0xc0) == 0x80; }

int utf8_chrlen(const char* s, int pos) {
    int len = 1;
    while (*(++s)) {
        if (utf8_is_continuation(*s)) {
            ++len;
        } else {
            return len;
        }
    }
    return len;
}

int utf8_strlen(const char* s) {
    int len = 0;
    while (*s) {
        if (!utf8_is_continuation(*s)) len++;
        s++;
    }
    return len;
}

int utf8_string_to_wstring(char* text, wchar_t* wtext, int max_bytes) {
    char* pt = text;
    wchar_t* wpt = wtext;
    int clength;
    int wstring_length = 0;
    mbstate_t mbs;
    memset(&mbs, 0, sizeof(mbs));

    while (max_bytes > 0) {
        clength = mbrtowc(wpt++, pt, max_bytes, &mbs);
        if ((clength == 0) || (clength > max_bytes)) {
            break;
        }
        pt += clength;
        max_bytes -= clength;
        ++wstring_length;
    }

    return wstring_length;
}
