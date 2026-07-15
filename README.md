# Split-flap departures

This project aims to display a train departure board with a split-flap effect in the terminal, all written in C.

## Building the program

This program currently only supports Linux and MacOS, and probably other POSIX OSes (untested).
To build the program, you need the libraries **ncurses**, **jansson** and **libcurl** to be installed.

Simply run `make` in the root directory and then run `./splitflap` to use the program.

## Configuration

Currently, the program simply displays the real-time departure board of an SNCF station.
The program needs a *config.json* file containing an SNCF realtime API key to work properly, with the following format:
```json
{
    "sncf_auth_key": "YOUR_API_KEY_HERE"
}
```
