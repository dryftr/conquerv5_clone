# Working Makefile for Original Conquer V5 (with SDL2)
CC = gcc
CFLAGS = -Wall -g -O0 -DDEBUG -I./Include -I../Include -DBSD -DPLATFORM_LINUX \
         -DDEFAULTDIR=\"/home/rayray/conquerv5_clone/original\" \
         -DEXEDIR=\"/home/rayray/conquerv5_clone/original\" \
         -DCONQ_SORT=\"/home/rayray/conquerv5_clone/original/conqsort\" \
         -I./sdl2 -lSDL2 -lSDL2_ttf

# All source files except mainA.c (for conquer) + mainG.c (for conqrun)
GAME_OBJS = $(filter-out mainA.o, $(wildcard *.c:.c=.o) sdl2/SDL2_display.o)
ADMIN_OBJS = $(filter-out mainG.o, $(wildcard *.c:.c=.o) sdl2/SDL2_display.o) stubs.o

# SDL2 display object
SDL2_DISPLAY_OBJS = sdl2/SDL2_display.o

all: conquer conqrun

conquer: mainG.o $(GAME_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses -lcrypt

conqrun: mainA.o $(ADMIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses -lcrypt

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o sdl2/SDL2_display.o conquer conqrun

.PHONY: clean all

# SDL2 Display Layer (Phase 4)
# Note: Add -lSDL2 -lSDL2_ttf to CFLAGS when you link against SDL2_display.o
# Example: conquer: mainG.o $(GAME_OBJS)
#          $(CC) $(CFLAGS) -o $@ $^ -lncurses -lcrypt -lSDL2 -lSDL2_ttf

