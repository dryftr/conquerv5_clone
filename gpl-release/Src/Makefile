# Working Makefile for Conquer V5
CC = gcc
CFLAGS = -Wall -g -O0 -DDEBUG -I./Include -I../Include -DBSD -DPLATFORM_LINUX -DDEFAULTDIR=\"/home/rayray/conquerv5_clone/gpl-release\" -DEXEDIR=\"/home/rayray/conquerv5_clone/gpl-release\" -DCONQ_SORT=\"/home/rayray/conquerv5_clone/gpl-release/conqsort\"

# All source files except mainA.c (for conquer)
GAME_OBJS = $(filter-out mainA.o, $(wildcard *.c:.c=.o))

# All source files except mainG.c + stubs.o (for conqrun)
ADMIN_OBJS = $(filter-out mainG.o, $(wildcard *.c:.c=.o)) stubs.o

all: conquer conqrun

conquer: mainG.o $(GAME_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses -lcrypt

conqrun: mainA.o $(ADMIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses -lcrypt

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o conquer conqrun

.PHONY: clean all
