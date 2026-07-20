CC=gcc
C_FLAGS=-g -Wall -Wextra -Wno-unused-parameter -Werror=return-type -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DHTTP_ONLY
LIBS=-lncursesw -ltinfo -ljansson -pthread -lm -lcurl
LD_FLAGS=
SRC=audio.c config.c utils.c net.c update.c json.c api_sncf.c gui.c main.c

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	C_FLAGS += -D LINUX
	LIBS += -ldl
endif
ifeq ($(UNAME_S),Darwin)
	C_FLAGS += -D MACOS
	C_FLAGS += -I/opt/homebrew/include
	LD_FLAGS += -L/opt/homebrew/lib
endif

all: splitflap

splitflap: $(SRC) miniaudio.o
	$(CC) --std=c99 $(C_FLAGS) $(LD_FLAGS) -o splitflap $(SRC) miniaudio.o $(LIBS)

miniaudio.o: miniaudio.c
	$(CC) --std=c99 -static -Os -c miniaudio.c -o miniaudio.o
