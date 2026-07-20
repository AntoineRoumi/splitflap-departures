#include "gui.h"

#include <assert.h>
#include <bits/time.h>
#include <bits/types/struct_timeval.h>
#include <curses.h>
#include <locale.h>
#include <ncursesw/curses.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#include "api_sncf.h"
#include "audio.h"
#include "config.h"
#include "utils.h"

#define GUI_TOP_OFFSET 1
#define GUI_STATION_NAME_HEIGHT 2
#define GUI_STATION_NAME_ROW (GUI_TOP_OFFSET)
#define GUI_HEADER_HEIGHT 2
#define GUI_HEADER_ROW (GUI_STATION_NAME_ROW + GUI_STATION_NAME_HEIGHT)
#define GUI_DEPARTURES_ROW (GUI_HEADER_ROW + GUI_HEADER_HEIGHT)
#define GUI_LEFT_PADDING 2
#define GUI_TIME_OFFSET (GUI_LEFT_PADDING)
#define GUI_TIME_LENGTH 7
#define GUI_DELAY_OFFSET (GUI_TIME_OFFSET + GUI_TIME_LENGTH)
#define GUI_DELAY_LENGTH 10
#define GUI_LINE_OFFSET (GUI_DELAY_OFFSET + GUI_DELAY_LENGTH)
#define GUI_LINE_LENGTH 16
#define GUI_TRAIN_NUMBER_OFFSET (GUI_LINE_OFFSET + GUI_LINE_LENGTH)
#define GUI_TRAIN_NUMBER_LENGTH 10
#define GUI_DESTINATION_OFFSET \
    (GUI_TRAIN_NUMBER_OFFSET + GUI_TRAIN_NUMBER_LENGTH)
#define GUI_DESTINATION_LENGTH 64
#define GUI_TOTAL_LINE_LENGTH                               \
    (GUI_TIME_LENGTH + GUI_DELAY_LENGTH + GUI_LINE_LENGTH + \
     GUI_TRAIN_NUMBER_LENGTH + GUI_DESTINATION_LENGTH)

#define GUI_ROW_HEIGHT 2
#define GUI_ROW_HEIGHT_INDEX(i) (GUI_DEPARTURES_ROW + GUI_ROW_HEIGHT * (i))

#define GUI_LINE_BUFFER_SIZE 512

#define GUI_STATION_NAME_LENGTH 64
#define GUI_STATION_NAME_ENTRY_TOP 1
#define GUI_STATION_NAME_ENTRY_OFFSET 2
#define GUI_STATION_NAME_ENTRY_TEXT "Station name: "

#define GUI_SPLITFLAP_FRAME_DURATION (1.f / g_config.splitflap_fps)

#define GUI_INPUT_UPDATE_FREQUENCY 60  // > 1 !!!
#define GUI_INPUT_UPDATE_INTERVAL_NANO \
    (long long)(1e9 * (1.f / GUI_INPUT_UPDATE_FREQUENCY))

static struct timespec input_update_interval = {0};

static wchar_t wstring_buffer[512];

typedef struct _departures_display {
    size_t n_lines;
    int** char_indices;
    wchar_t** lines;
} departures_display;

static void gui_init_departures_display(int n_lines,
                                        departures_display* display) {
    display->n_lines = n_lines;
    display->lines = malloc(n_lines * sizeof(char*));
    display->char_indices = malloc(n_lines * sizeof(int*));
    for (int i = 0; i < n_lines; ++i) {
        display->char_indices[i] = malloc(GUI_TOTAL_LINE_LENGTH * sizeof(int));
        for (int j = 0; j < GUI_TOTAL_LINE_LENGTH; ++j) {
            display->char_indices[i][j] = 0;
        }
        display->lines[i] = malloc(GUI_TOTAL_LINE_LENGTH * sizeof(wchar_t));
        for (int j = 0; j < GUI_TOTAL_LINE_LENGTH; ++j) {
            display->lines[i][j] = L' ';
        }
    }
}

static const wchar_t french_characters[] =
    L" ABCDEFGHIJKLMNOPQRSTUVWXYZÀÂÄÇÉÈÊËÎÏÔÙÛÜabcdefghijklmnopqrstuvwxyzàâäçéè"
    L"êëîïôùûü"
    L"ïôùû0123456789+-'";
static size_t n_french_characters;
static const wchar_t digits[] = L" 0123456789";
static size_t n_digits = (sizeof(digits) - 1) / sizeof(wchar_t);

static int index_of_wchar_french(wchar_t c) {
    for (size_t i = 0; i < n_french_characters; i++) {
        if (french_characters[i] == c) {
            return (int)i;
        }
    }
    return -1;
}

static int index_of_digit(wchar_t c) {
    for (size_t i = 0; i < n_digits; ++i) {
        if (digits[i] == c) {
            return (int)i;
        }
    }
    return -1;
}

static void generate_indices_from_departure_line(wchar_t line[], int indices[],
                                                 int line_length) {
    indices[0] = index_of_digit(line[0]);
    indices[1] = index_of_digit(line[1]);
    indices[2] = 0;
    indices[3] = index_of_digit(line[3]);
    indices[4] = index_of_digit(line[4]);

    for (int i = 5; i < line_length; ++i) {
        indices[i] = index_of_wchar_french(line[i]);
    }
}

static departures_display target_display = {0};
static departures_display current_display = {0};

void gui_init() {
    setlocale(LC_ALL, "fr_FR.UTF-8");
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    nodelay(stdscr, TRUE);

    if (has_colors()) {
        fprintf(stderr, "error: this terminal does not support color\n");
    }
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    gui_init_departures_display(10, &current_display);
    gui_init_departures_display(10, &target_display);

    n_french_characters = wcslen(french_characters);

    input_update_interval = (struct timespec){
        .tv_sec = 0, .tv_nsec = GUI_INPUT_UPDATE_INTERVAL_NANO};
}

char gui_process_event() {
    int c = getch();

    // Process some special gui events first
    switch (c) {
        case KEY_RESIZE:
            break;
        case KEY_MOUSE:
            break;
    }

    return c;
}

void gui_sleep() { nanosleep(&input_update_interval, NULL); }

void gui_generate_departures_display(sncf_departure_table* departures,
                                     departures_display* display) {
    size_t displayed_departures = (departures->n_departures > display->n_lines)
                                      ? display->n_lines
                                      : departures->n_departures;
    sncf_departure* dep;
    wchar_t* line;
    for (size_t i = 0; i < displayed_departures; ++i) {
        dep = &(departures->departures[i]);
        line = display->lines[i];

        static char delay_string[GUI_DELAY_LENGTH];
        if (dep->delay <= 0) {
            strcpy(delay_string, "On time");
        } else {
            snprintf(delay_string, GUI_DELAY_LENGTH, "+%d'", dep->delay);
        }

        utf8_string_to_wstring(dep->dest, wstring_buffer,
                               GUI_DESTINATION_LENGTH);
        swprintf(line, 512, L"%02d:%02d%*s%s%*s%s%*s%s%*s%ls%*s",
                 dep->dep_time.hour, dep->dep_time.minute,
                 (5 - GUI_TIME_LENGTH), " ", delay_string,
                 (strlen(delay_string) - GUI_DELAY_LENGTH), " ", dep->line,
                 (strlen(dep->line) - GUI_LINE_LENGTH), " ", dep->train_number,
                 (strlen(dep->train_number) - GUI_TRAIN_NUMBER_LENGTH), " ",
                 wstring_buffer,
                 (wcslen(wstring_buffer) - GUI_DESTINATION_LENGTH), " ");

        generate_indices_from_departure_line(line, display->char_indices[i],
                                             GUI_TOTAL_LINE_LENGTH);
    }

    // Fill the empty lines with whitespaces
    for (size_t i = displayed_departures; i < display->n_lines; ++i) {
        wmemset(display->lines[i], L' ', GUI_TOTAL_LINE_LENGTH);
    }
}

void gui_copy_departures_display(departures_display* dst, departures_display* src) {
    assert(dst->n_lines == src->n_lines);

    for (size_t i_line = 0; i_line < src->n_lines; ++i_line) {
        memcpy(dst->lines[i_line], src->lines[i_line], sizeof(wchar_t) * GUI_TOTAL_LINE_LENGTH);
        memcpy(dst->char_indices[i_line], src->char_indices[i_line], sizeof(int) * GUI_TOTAL_LINE_LENGTH);
    }
}

// Render the next frame of splitflap animation and returns the number of flips
// during that frame (0 if old_display == new_display)
int gui_splitflap_frame(departures_display* old_display,
                        departures_display* new_display) {
    assert(old_display->n_lines == new_display->n_lines);

    int total_changes = 0;

    wchar_t* old_line;
    int *new_indices, *old_indices;
    for (size_t i_line = 0; i_line < new_display->n_lines; ++i_line) {
        old_line = old_display->lines[i_line];
        new_indices = new_display->char_indices[i_line];
        old_indices = old_display->char_indices[i_line];

        // For time
        for (size_t i_char = 0; i_char < 5; ++i_char) {
            if (i_char == 2) {
                old_line[i_char] = L':';
            } else if (new_indices[i_char] != old_indices[i_char]) {
                old_indices[i_char] = (old_indices[i_char] + 1) % n_digits;
                old_line[i_char] = digits[old_indices[i_char]];
                ++total_changes;
            }
        }

        for (size_t i_char = 5; i_char < GUI_TOTAL_LINE_LENGTH; ++i_char) {
            if (new_indices[i_char] == -1) {
                old_indices[i_char] = -1;
                old_line[i_char] = new_display->lines[i_line][i_char];
            }

            if (new_indices[i_char] != old_indices[i_char]) {
                old_indices[i_char] =
                    (old_indices[i_char] + 1) % n_french_characters;
                old_line[i_char] = french_characters[old_indices[i_char]];
                ++total_changes;
            }
        }
    }

    return total_changes;
}

// Remember to free the string after use
void gui_station_name_entry(sncf_station* station) {
    clear();

    char* station_name = malloc(GUI_STATION_NAME_LENGTH + 1);
    int current_length = 0;

    int number_of_results = 0;
    sncf_station* autocomplete_stop_areas = malloc(10 * sizeof(sncf_station));
    bool name_changed = false;

    mvprintw(GUI_STATION_NAME_ENTRY_TOP, GUI_STATION_NAME_ENTRY_OFFSET,
             GUI_STATION_NAME_ENTRY_TEXT);

    int c;
    struct timeval time_stop, time_start;
    uint64_t delta_time;
    do {
        c = getch();

        if (c >= 32 && c <= 126 && current_length < GUI_STATION_NAME_LENGTH) {
            station_name[current_length++] = c;
            station_name[current_length] = '\0';
            name_changed = true;
            gettimeofday(&time_start, NULL);
        } else if (c == KEY_BACKSPACE && current_length != 0) {
            clear();
            station_name[--current_length] = '\0';
            name_changed = true;
            gettimeofday(&time_start, NULL);
        }

        if (name_changed) {
            gettimeofday(&time_stop, NULL);
            delta_time = (time_stop.tv_sec - time_start.tv_sec);

            if ((c == '\n' || delta_time >= 1) && current_length > 0) {
                clear();
                number_of_results = sncf_autocomplete_stop_area(
                    station_name, 10, &autocomplete_stop_areas);
                name_changed = false;
            } else if (current_length == 0) {
                clear();
                number_of_results = 0;
                name_changed = false;
            }

            for (int i = 0; i < number_of_results; i++) {
                utf8_string_to_wstring(autocomplete_stop_areas[i].name,
                                       wstring_buffer,
                                       GUI_STATION_NAME_LENGTH + 1);
                mvaddwstr(GUI_STATION_NAME_ENTRY_TOP + 2 * (i + 1),
                          GUI_STATION_NAME_ENTRY_OFFSET, wstring_buffer);
            }

            mvprintw(GUI_STATION_NAME_ENTRY_TOP, GUI_STATION_NAME_ENTRY_OFFSET,
                     "%s%s", GUI_STATION_NAME_ENTRY_TEXT, station_name);

            move(GUI_STATION_NAME_ENTRY_TOP,
                 GUI_STATION_NAME_ENTRY_OFFSET +
                     strlen(GUI_STATION_NAME_ENTRY_TEXT) + current_length);

            refresh();
        }
    } while (c != '\n');

    // Station selector
    if (number_of_results == 0) {
        return;
    }

    int selected_option = 0;
    bool is_selected;
    do {
        c = getch();

        switch (c) {
            case KEY_DOWN:
                if (selected_option < number_of_results - 1) {
                    ++selected_option;
                }
                break;
            case KEY_UP:
                if (selected_option > 0) {
                    --selected_option;
                }
                break;
        }

        for (int i = 0; i < number_of_results; i++) {
            if ((is_selected = (i == selected_option))) {
                attron(COLOR_PAIR(1));
            }

            utf8_string_to_wstring(autocomplete_stop_areas[i].name,
                                   wstring_buffer, GUI_STATION_NAME_LENGTH + 1);
            mvaddwstr(GUI_STATION_NAME_ENTRY_TOP + 2 * (i + 1),
                      GUI_STATION_NAME_ENTRY_OFFSET, wstring_buffer);

            if (is_selected) {
                attroff(COLOR_PAIR(1));
            }
        }

        mvprintw(GUI_STATION_NAME_ENTRY_TOP, GUI_STATION_NAME_ENTRY_OFFSET,
                 "%s%s", GUI_STATION_NAME_ENTRY_TEXT, station_name);

        move(GUI_STATION_NAME_ENTRY_TOP,
             GUI_STATION_NAME_ENTRY_OFFSET +
                 strlen(GUI_STATION_NAME_ENTRY_TEXT) + current_length);
    } while (c != '\n');

    strcpy(station->id, autocomplete_stop_areas[selected_option].id);
    strcpy(station->name, autocomplete_stop_areas[selected_option].name);
}

void gui_update_display() {
    if (!g_departure_table_updated) {
        return;
    }

    pthread_mutex_lock(&g_departure_table_mutex);

    gui_generate_departures_display(&g_departure_table, &target_display);
    g_departure_table_updated = false;

    pthread_mutex_unlock(&g_departure_table_mutex);

    gui_render_departures(&g_departure_table);
}

void gui_render_departures(sncf_departure_table* departure_table) {
    clear();

    utf8_string_to_wstring(g_current_station.name, wstring_buffer,
                           GUI_STATION_NAME_LENGTH + 1);
    mvaddwstr(GUI_STATION_NAME_ROW, GUI_LEFT_PADDING, wstring_buffer);

    mvprintw(GUI_HEADER_ROW, GUI_TIME_OFFSET, "Time");
    mvprintw(GUI_HEADER_ROW, GUI_DELAY_OFFSET, "Delay");
    mvprintw(GUI_HEADER_ROW, GUI_LINE_OFFSET, "Name");
    mvprintw(GUI_HEADER_ROW, GUI_TRAIN_NUMBER_OFFSET, "No");
    mvprintw(GUI_HEADER_ROW, GUI_DESTINATION_OFFSET, "Destination");

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    struct timespec time_start, time_stop;
    int total_changes = GUI_TOTAL_LINE_LENGTH;
    struct timespec remaining_time = {0};
    struct timespec target_time = {
        .tv_sec = (time_t)GUI_SPLITFLAP_FRAME_DURATION,
        .tv_nsec = (GUI_SPLITFLAP_FRAME_DURATION -
                    (time_t)GUI_SPLITFLAP_FRAME_DURATION) *
                   1e9};

    audio_start_splitflap();

    int c;
    while (1) {
        clock_gettime(CLOCK_REALTIME, &time_start);

        // Handle input
        while ((c = getch()) != ERR) {
            if (c == ' ' || c == 'q') {
                gui_copy_departures_display(&current_display, &target_display);
                total_changes = 0;
            }
        }

        if (total_changes != 0) {
            total_changes = gui_splitflap_frame(&current_display, &target_display);
        }

        for (size_t i = 0; i < current_display.n_lines; ++i) {
            if (i % 2 == 0) {
                attron(COLOR_PAIR(1));
            }
            mvaddwstr(GUI_ROW_HEIGHT_INDEX(i), GUI_TIME_OFFSET,
                      current_display.lines[i]);
            if (i % 2 == 0) {
                attroff(COLOR_PAIR(1));
            }
        }

        refresh();

        if (total_changes == 0) {
            goto _exit_gui_render_departures;
        }

        clock_gettime(CLOCK_REALTIME, &time_stop);

        remaining_time.tv_sec =
            target_time.tv_sec - (time_stop.tv_sec - time_start.tv_sec);
        remaining_time.tv_nsec =
            target_time.tv_nsec - (time_stop.tv_nsec - time_start.tv_nsec);
        nanosleep(&remaining_time, NULL);
    }

_exit_gui_render_departures:

    audio_stop_splitflap();
}

void gui_terminate() { endwin(); }
