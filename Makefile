CC=gcc
C_FLAGS=-g -Wall -Wextra -Wno-unused-parameter -Werror=return-type
LIBS=-lncurses -lcdk -lcurl -ljansson
LD_FLAGS=
SRC=config.c utils.c net.c update.c json.c api_sncf.c gui.c main.c

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	C_FLAGS += -D LINUX 
endif
ifeq ($(UNAME_S),Darwin)
	C_FLAGS += -D MACOS
	C_FLAGS += -I/opt/homebrew/include
	LD_FLAGS += -L/opt/homebrew/lib
endif

splitflap: $(SRC)
	$(CC) --std=c99 $(C_FLAGS) $(LD_FLAGS) $(LIBS) -o splitflap $(SRC)
