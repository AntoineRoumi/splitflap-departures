# Split-flap departures

This project aims to display a train departure board with a split-flap effect in the terminal, all written in C.

## Building the program

This program currently supports Linux and MacOS, and probably other POSIX OSes (untested). No Windows support is planned.
To build the program, you need the libraries **ncurses**, **jansson** and **libcurl** to be installed.

Simply run `make` in the root directory and then run `./splitflap` to use the program.

## Configuration

As the application makes calls to the SNCF realtime API, you must create a file named *config.json* containing an SNCF realtime API key to work properly.
Currently, providing no API key will crash the application.

You can also specify the update interval (in seconds) for the departure board and the speed of the splitflap effect.

### *config.json*
```json
{
    "sncf_auth_key": "YOUR_API_KEY_HERE",
    "splitflap_fps": 20,  
    "update_interval": 60
}
```

## Key bindings

- Press the `S` key to open the station search engine.
- Press the `Q` key to quit the application.
