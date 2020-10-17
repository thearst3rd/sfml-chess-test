# Makefile for SFML chess board implemenetation in C
# by thearst3rd

ifndef (CHESSLIB_DIR)
	CHESSLIB_DIR = ../chesslib
endif

CC = gcc
CFLAGS = -Wall -I$(CHESSLIB_DIR)/include

ifeq ($(DEBUG),1)
	CFLAGS += -g
else
	CFLAGS += -g0
endif

SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

LIBS = -lm -lcsfml-graphics -lcsfml-window -lcsfml-system -lcsfml-audio

# Platform independance
ifeq ($(OS), Windows_NT)
	EXE = bin/sfml-app.exe
	CFLAGS += -mwindows
else
	EXE = bin/sfml-app
endif


all: sfml-app

sfml-app: $(EXE)

chesslib: $(CHESSLIB_DIR)/bin/libchesslib.a

$(CHESSLIB_DIR)/bin/libchesslib.a:
	make -C $(CHESSLIB_DIR) chesslib


$(ALLOBJECTS): src/%.o : src/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(EXE): $(OBJECTS) chesslib | bin
	$(CC) $(CFLAGS) -o $(EXE) $(OBJECTS) $(LIBS) -L$(CHESSLIB_DIR)/bin -lchesslib


bin:
	@mkdir -p bin

clean:
	rm -rf **/*.o bin

run: $(EXE)
	./$(EXE)
