# Working Makefile for Conquer V5
CC = gcc
CFLAGS = -Wall -g -O0 -DDEBUG -I./Include -I../Include -DBSD -DPLATFORM_LINUX \
         -DDEFAULTDIR=\"/home/rayray/conquerv5_clone/gpl-release\" \
         -DEXEDIR=\"/home/rayray/conquerv5_clone/gpl-release\" \
         -DCONQ_SORT=\"/home/rayray/conquerv5_clone/gpl-release/conqsort\"

# Optional SDL2 libraries – added when building with USE_SDL2
SDL_LIBS =
SDL_CFLAGS =
ifdef USE_SDL2
SDL_LIBS = -lSDL2 -lSDL2_ttf -lm
SDL_CFLAGS = -DUSE_SDL2
endif

# All source files except mainA.c (for conquer) + mainG.c (for conqrun)
SRCS_GAME = $(filter-out main_sdl2.c,$(wildcard *.c))
OBJS_GAME = $(patsubst %.c,%.o,$(SRCS_GAME))
GAME_OBJS = $(filter-out mainA.o, $(OBJS_GAME))

SRCS_ADMIN = $(wildcard *.c)
OBJS_ADMIN = $(patsubst %.c,%.o,$(SRCS_ADMIN))
ADMIN_OBJS = $(filter-out mainG.o, $(OBJS_ADMIN)) stubs.o

# SDL2 display objects (Phase 4)
SDL2_DISPLAY_OBJS = sdl2_display.o displayG_sdl2.o

# Phase 4c: Hex map objects
HEXMAP_OBJS = hexmapG_sdl2.o

# Phase 4d/5b: Sprite loader objects
SPRITE_OBJS = sprite_loader.o

# Phase 5b: Hexmap with sprites
HEXMAP_SPRITES_OBJS = hexmapG_sprites.o

all: conquer conqrun

conquer: mainG.o $(filter-out main_sdl2.o,$(GAME_OBJS)) $(SDL2_DISPLAY_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(SDL_LIBS) -lncurses -lcrypt

conqrun: mainA.o $(filter-out main_sdl2.o,$(ADMIN_OBJS)) $(SDL2_DISPLAY_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(SDL_LIBS) -lncurses -lcrypt

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# SDL2 specific compilation (needs SDL2 headers)
sdl2_display.o: sdl2_display.c sdl2_display.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

displayG_sdl2.o: displayG_sdl2.c displayG_sdl2.h sdl2_display.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

hexmapG_sdl2.o: hexmapG_sdl2.c hexmapG_sdl2.h sdl2_display.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

# SDL2 test targets
test-sdl2: USE_SDL2 = 1
test-sdl2: test_sdl2
test_sdl2: main_sdl2.c sdl2_display.o
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ $(SDL_LIBS)

# Phase 4c: Hex map test
test-hexmap: test_hexmap
test_hexmap: test_hexmap_sdl2.c sdl2_display.o hexmapG_sdl2.o
	$(CC) $(CFLAGS) -DUSE_SDL2 -o $@ $^ -lSDL2 -lSDL2_ttf -lm
	@echo "=== Hex Map Test Built ==="
	@echo "Run: ./test_hexmap"
	@echo "Controls: Arrow keys/HJKL = move cursor, WASD = pan view"
	@echo "          1-4 = display modes, +/- = zoom, c = center, r = regen"

# Phase 4d: Sprite loader test
test-sprites: test_sprite_loader
test_sprite_loader: test_sprite_loader.c sdl2_display.o hexmapG_sdl2.o sprite_loader.o
	$(CC) $(CFLAGS) -DUSE_SDL2 -o $@ $^ -lSDL2 -lSDL2_ttf -lSDL2_image -lm
	@echo "=== Sprite Loader Test Built ==="
	@echo "Run: ./test_sprite_loader [-r WxH]"
	@echo "Controls: 1-4 = display modes, +/- = zoom, WASD = pan"
	@echo "          Arrow/HJKL = cursor, R = reload sprites, ESC = quit"
	@echo "Sprites: drop PNGs in sprites/ subdirs to customize!"

# Phase 5b: Hexmap with sprites test
test-hexmap-sprites: test_hexmap_sprites
test_hexmap_sprites: test_hexmap_sprites.c sdl2_display.o hexmapG_sdl2.o hexmapG_sprites.o sprite_loader.o
	$(CC) $(CFLAGS) -DUSE_SDL2 -o $@ $^ -lSDL2 -lSDL2_ttf -lSDL2_image -lm -ljson-c
	@echo "=== Hexmap Sprites Test Built ==="
	@echo "Run: ./test_hexmap_sprites [-r WxH]"
	@echo "Controls: Arrow/HJKL = cursor, WASD = pan, +/- = zoom"
	@echo "          1-4 = display modes, F = toggle sprites, V/B/U = toggle layers"
	@echo "          R = reload sprites, Q/ESC = quit"

# Sprite loader compilation
sprite_loader.o: sprite_loader.c sprite_loader.h sdl2_display.h
	$(CC) $(CFLAGS) -DUSE_SDL2 -c $< -o $@

# Hexmap with sprites compilation
hexmapG_sprites.o: hexmapG_sprites.c hexmapG_sprites.h hexmapG_sdl2.h sprite_loader.h
	$(CC) $(CFLAGS) -DUSE_SDL2 -c $< -o $@

# SDL2 builds
sdl2: USE_SDL2 = 1
sdl2: clean conquer conqrun

sdl2-800: USE_SDL2 = 1
sdl2-800: clean
sdl2-800: conquer conqrun
	sdl2-1024: USE_SDL2 = 1
sdl2-1024: clean
sdl2-1024: conquer conqrun

clean:
	rm -f *.o $(SDL2_DISPLAY_OBJS) $(HEXMAP_OBJS) $(SPRITE_OBJS) $(HEXMAP_SPRITES_OBJS) conquer conqrun test_sdl2 test_hexmap test_sprite_loader test_hexmap_sprites

.PHONY: clean all sdl2 sdl2-800 sdl2-1024 test-sdl2 test-hexmap test-sprites test-hexmap-sprites
