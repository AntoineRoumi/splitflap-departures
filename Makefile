CC=gcc
C_FLAGS=--std=c99 -Wall -Wextra -Wno-unused-parameter -Werror=return-type -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DHTTP_ONLY
DEBUG_C_FLAGS=-g
RELEASE_C_FLAGS=-O1
LIBS=-ljansson -pthread -lm -lcurl
LD_FLAGS=
MINIAUDIO_FLAGS=
SRC=audio.c config.c utils.c net.c update.c json.c api_sncf.c gui.c main.c

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	MINIAUDIO_FLAGS += -D MA_ENABLE_PULSEAUDIO -D MA_ENABLE_ALSA -D MA_ENABLE_JACK
	C_FLAGS += -D LINUX
	LIBS += -ldl -lncursesw -ltinfo
endif
ifeq ($(UNAME_S),Darwin)  # macos
	MINIAUDIO_FLAGS += -D MA_ENABLE_COREAUDIO
	C_FLAGS += -D MACOS 
	C_FLAGS += -I/opt/homebrew/include
	LD_FLAGS += -L/opt/homebrew/lib
	LIBS += -lncurses
endif

all: splitflap-release

splitflap-release: $(SRC) miniaudio.o
	$(CC) $(RELEASE_C_FLAGS) $(C_FLAGS) $(LD_FLAGS) -o splitflap $(SRC) miniaudio.o $(LIBS) 

splitflap-debug: $(SRC) miniaudio.o
	$(CC) $(DEBUG_C_FLAGS) $(C_FLAGS) $(LD_FLAGS) -o splitflap-debug $(SRC) miniaudio.o $(LIBS)

miniaudio.o: miniaudio.c
	$(CC) $(MINIAUDIO_FLAGS) -Os -c miniaudio.c -o miniaudio.o
