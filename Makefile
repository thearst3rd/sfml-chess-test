# Makefile for SFML chess board implemenetation in C
# by thearst3rd

CC = gcc
CFLAGS = -Wall -I../chesslib/include

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


$(ALLOBJECTS): src/%.o : src/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(EXE): $(OBJECTS) | bin
	$(CC) $(CFLAGS) -o $(EXE) $(OBJECTS) $(LIBS) -L../chesslib/bin -lchesslib


bin:
	@mkdir -p bin

clean:
	rm -rf **/*.o bin

run: $(EXE)
	./$(EXE)
