#include "gui.h"

#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <ncursesw/curses.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <wchar.h>

#include "api_sncf.h"
#include "utils.h"

#define GUI_TOP_OFFSET 1
#define GUI_STATION_NAME_HEIGHT 2
#define GUI_STATION_NAME_ROW (GUI_TOP_OFFSET)
#define GUI_HEADER_HEIGHT 2
#define GUI_HEADER_ROW (GUI_STATION_NAME_ROW + GUI_STATION_NAME_HEIGHT)
#define GUI_DEPARTURES_ROW (GUI_HEADER_ROW + GUI_HEADER_HEIGHT)
#define GUI_ROW_HEIGHT 2
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

#define GUI_LINE_BUFFER_SIZE 512

#define GUI_STATION_NAME_LENGTH 64
#define GUI_STATION_NAME_ENTRY_TOP 1
#define GUI_STATION_NAME_ENTRY_OFFSET 2
#define GUI_STATION_NAME_ENTRY_TEXT "Station name: "

static wchar_t wstring_buffer[512];

static void gui_init_departures_display(int n_lines,
                                        departures_display* display) {
    display->n_lines = 0;
    display->max_n_lines = n_lines;
    display->lines = malloc(n_lines * sizeof(char*));
    for (int i = 0; i < n_lines; ++i) {
        display->lines[i] = malloc(GUI_LINE_BUFFER_SIZE);
    }
}

static const wchar_t french_characters[] =
    L" ABCDEFGHIJKLMNOPQRSTUVWXYZÀÂÇÉÈÊËÎÏÔÙÛabcdefghijklmnopqrstuvwxyzàâçéèêëî"
    L"ïôùû0123456789-'";
static size_t n_french_characters;
static const char digits[] = "0123456789";
static size_t n_digits = sizeof(digits) - 1;

static departures_display ddisplay;

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

    gui_init_departures_display(10, &ddisplay);

    n_french_characters = wcslen(french_characters);
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

int gui_compute_row_height(size_t index) {
    return GUI_DEPARTURES_ROW + GUI_ROW_HEIGHT * index;
}

void gui_generate_departures_display(sncf_departure_table* departures,
                                     departures_display* display) {
    display->n_lines = (departures->n_departures > display->max_n_lines)
                           ? display->max_n_lines
                           : departures->n_departures;

    sncf_departure* dep;
    wchar_t* line;
    for (size_t i = 0; i < display->n_lines; ++i) {
        dep = &(departures->departures[i]);
        line = display->lines[i];

        static char delay_string[GUI_DELAY_LENGTH];
        if (dep->delay <= 0) {
            strcpy(delay_string, "On time");
        } else {
            sprintf(delay_string, "+%d'", dep->delay);
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
    }
}

int gui_splitflap_animate_frame(departures_display* old_display,
                           departures_display* new_display) {
    assert(old_display->max_n_lines == new_display->max_n_lines);

    for (size_t i_line = 0; i_line < new_display->n_lines; ++i_line) {
        // TODO
    }

    return 0;
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

void gui_update_departures() {
    if (!g_departure_table_updated) {
        gui_display_departures(&g_departure_table);
    }

    pthread_mutex_lock(&g_departure_table_mutex);

    gui_display_departures(&g_departure_table);
    g_departure_table_updated = false;

    pthread_mutex_unlock(&g_departure_table_mutex);
}

void gui_display_departures(sncf_departure_table* departure_table) {
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

    int row_y;

    gui_generate_departures_display(departure_table, &ddisplay);

    for (size_t i = 0; i < ddisplay.n_lines; ++i) {
        if (i % 2 == 0) {
            attron(COLOR_PAIR(1));
        }
        row_y = gui_compute_row_height(i);
        mvaddwstr(row_y, GUI_TIME_OFFSET, ddisplay.lines[i]);
        if (i % 2 == 0) {
            attroff(COLOR_PAIR(1));
        }
    }

    refresh();
}

void gui_terminate() { endwin(); }
