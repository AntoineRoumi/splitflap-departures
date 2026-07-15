#include "gui.h"

#include <ncurses.h>
#include <pthread.h>
#include <string.h>

#include "api_sncf.h"

#define GUI_TOP_OFFSET 1
#define GUI_STATION_NAME_HEIGHT 2
#define GUI_STATION_NAME_ROW (GUI_TOP_OFFSET)
#define GUI_HEADER_HEIGHT 2
#define GUI_HEADER_ROW (GUI_STATION_NAME_ROW + GUI_STATION_NAME_HEIGHT)
#define GUI_DEPARTURES_ROW (GUI_HEADER_ROW + GUI_HEADER_HEIGHT)
#define GUI_ROW_HEIGHT 2
#define GUI_LEFT_PADDING 2
#define GUI_TIME_OFFSET (GUI_LEFT_PADDING)
#define GUI_DELAY_OFFSET (GUI_TIME_OFFSET + 7)
#define GUI_LINE_OFFSET (GUI_DELAY_OFFSET + 10)
#define GUI_TRAIN_NUMBER_OFFSET (GUI_LINE_OFFSET + 16)
#define GUI_DESTINATION_OFFSET (GUI_TRAIN_NUMBER_OFFSET + 10)

#define GUI_STATION_NAME_LENGTH 95
#define GUI_STATION_NAME_ENTRY_TOP 1
#define GUI_STATION_NAME_ENTRY_OFFSET 2
#define GUI_STATION_NAME_ENTRY_TEXT "Station name: "

void gui_init() {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
}

char gui_process_event() {
    nodelay(stdscr, TRUE);
    int c = getch();

    // Process some special gui events first
    switch (c) {
        case KEY_RESIZE:
            gui_display_departures(NULL);
            break;
        case KEY_MOUSE:
            break;
    }

    return c;
}

int gui_compute_row_height(size_t index) {
    return GUI_DEPARTURES_ROW + GUI_ROW_HEIGHT * index;
}

// Remember to free the string after use
char* gui_station_name_entry() {
    clear();

    char* station_name = malloc(GUI_STATION_NAME_LENGTH + 1);
    int current_length = 0;

    mvprintw(GUI_STATION_NAME_ENTRY_TOP, GUI_STATION_NAME_ENTRY_OFFSET,
             GUI_STATION_NAME_ENTRY_TEXT);

    int c;
    do {
        c = getch();

        if (c >= 32 && c <= 126 && current_length < GUI_STATION_NAME_LENGTH) {
            mvaddch(GUI_STATION_NAME_ENTRY_TOP,
                    GUI_STATION_NAME_ENTRY_OFFSET +
                        strlen(GUI_STATION_NAME_ENTRY_TEXT) + current_length,
                    c);

            station_name[current_length++] = c;
        } else if (c == KEY_BACKSPACE && current_length != 0) {
            --current_length;
            mvaddch(GUI_STATION_NAME_ENTRY_TOP,
                    GUI_STATION_NAME_ENTRY_OFFSET +
                        strlen(GUI_STATION_NAME_ENTRY_TEXT) + current_length,
                    ' ');

            move(GUI_STATION_NAME_ENTRY_TOP,
                 GUI_STATION_NAME_ENTRY_OFFSET +
                     strlen(GUI_STATION_NAME_ENTRY_TEXT) + current_length);
        }
    } while (c != '\n');

    station_name[current_length] = '\0';

    return station_name;
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

    mvprintw(GUI_STATION_NAME_ROW, GUI_LEFT_PADDING, "%s",
             departure_table->station_name);

    mvprintw(GUI_HEADER_ROW, GUI_TIME_OFFSET, "Time");
    mvprintw(GUI_HEADER_ROW, GUI_DELAY_OFFSET, "Delay");
    mvprintw(GUI_HEADER_ROW, GUI_LINE_OFFSET, "Name");
    mvprintw(GUI_HEADER_ROW, GUI_TRAIN_NUMBER_OFFSET, "No");
    mvprintw(GUI_HEADER_ROW, GUI_DESTINATION_OFFSET, "Destination");

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    sncf_departure* dep;
    int row_y;
    for (size_t i = 0; i < departure_table->n_departures; ++i) {
        dep = &departure_table->departures[i];
        row_y = gui_compute_row_height(i);
        if (i % 2 == 0) {
            attron(COLOR_PAIR(1));
        }
        mvprintw(row_y, GUI_TIME_OFFSET, "%02d:%02d", dep->dep_time.hour,
                 dep->dep_time.minute);

        if (dep->delay <= 0) {
            mvprintw(row_y, GUI_DELAY_OFFSET, "On time");
        } else {
            mvprintw(row_y, GUI_DELAY_OFFSET, "+%d'", dep->delay);
        }

        mvprintw(row_y, GUI_LINE_OFFSET, "%s", dep->line);
        mvprintw(row_y, GUI_TRAIN_NUMBER_OFFSET, "%s", dep->train_number);
        mvprintw(row_y, GUI_DESTINATION_OFFSET, "%s", dep->dest);
        if (i % 2 == 0) {
            attroff(COLOR_PAIR(1));
        }
    }

    refresh();
}

void gui_terminate() { endwin(); }
