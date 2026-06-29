# Working Makefile for Conquer V5
CC = gcc
CFLAGS = -Wall -g -O0 -DDEBUG -I./Include -I../Include -DBSD -DPLATFORM_LINUX \
         -DDEFAULTDIR=\"/home/rayray/conquerv5_clone/gpl-release\" \
         -DEXEDIR=\"/home/rayray/conquerv5_clone/gpl-release\" \
         -DCONQ_SORT=\"/home/rayray/conquerv5_clone/gpl-release/conqsort\"

# SDL2 is now always linked for runtime display switching (gpl-release only).
# If SDL2 is not available at runtime, the game falls back to curses automatically.
SDL_LIBS = -lSDL2 -lSDL2_ttf -lSDL2_image -lm
SDL_CFLAGS = -DUSE_SDL2

# AI module sources (Sprint 0-1: personality, fog, decision, diplomacy, report, integration)
# Exclude test files from ai/ subdirectory
AI_SRCS = $(filter-out ai/test_%.c,$(wildcard ai/*.c))
AI_OBJS = $(patsubst ai/%.c,ai/%.o,$(AI_SRCS))

# Source files for conquer build (game) - exclude tests and alternate mains
SRCS_GAME = $(filter-out mainA.c main_sdl2.c test_%.c,$(wildcard *.c))
OBJS_GAME = $(patsubst %.c,%.o,$(SRCS_GAME))
# Filter out stubs.o, we'll use target-specific version
GAME_OBJS = $(filter-out mainA.o stubs.o,$(OBJS_GAME))

# Source files for conqrun build (admin) - exclude tests and game main
SRCS_ADMIN = $(filter-out mainG.c main_sdl2.c test_%.c,$(wildcard *.c))
OBJS_ADMIN = $(patsubst %.c,%.o,$(SRCS_ADMIN))
# Filter out stubs.o from common objs, we'll build target-specific versions
ADMIN_OBJS = $(filter-out mainG.o stubs.o,$(OBJS_ADMIN))

# SDL2 display objects (Phase 4)
SDL2_DISPLAY_OBJS = sdl2_display.o displayG_sdl2.o

# Phase 4c: Hex map objects
HEXMAP_OBJS = hexmapG_sdl2.o

# Phase 4d/5b: Sprite loader objects
SPRITE_OBJS = sprite_loader.o

# Phase 5b: Hexmap with sprites
HEXMAP_SPRITES_OBJS = hexmapG_sprites.o

all: conquer conqrun

conquer: CFLAGS += -DCONQUER_BUILD
conquer: mainG.o $(filter-out main_sdl2.o stubs.o,$(GAME_OBJS)) $(AI_OBJS) stubs_conquer.o $(SDL2_DISPLAY_OBJS) display_dispatch.o
	$(CC) $(CFLAGS) -o $@ $^ $(SDL_LIBS) -lncurses -lcrypt

conqrun: mainA.o $(filter-out main_sdl2.o,$(ADMIN_OBJS)) $(AI_OBJS) stubs_conqrun.o $(SDL2_DISPLAY_OBJS) display_dispatch.o
	$(CC) $(CFLAGS) -o $@ $^ $(SDL_LIBS) -lncurses -lcrypt

# Target-specific stub objects
stubs_conquer.o: stubs.c
	$(CC) $(CFLAGS) -DCONQUER_BUILD -c $< -o $@

stubs_conqrun.o: stubs.c
	$(CC) $(CFLAGS) -DCONQRUN_BUILD -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# AI module compilation (subdirectory)
ai/%.o: ai/%.c
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
	$(CC) $(CFLAGS) -DUSE_SDL2 -o $@ $^ -lSDL2 -lSDL2_ttf -lSDL2_image -lm
	@echo "=== Hexmap Sprites Test Built ==="
	@echo "Run: ./test_hexmap_sprites [-r WxH]"
	@echo "Controls: Arrow/HJKL = cursor, WASD = pan, +/- = zoom"
	@echo "          1-4 = display modes, F = toggle sprites, V/B/U = toggle layers"
	@echo "          M = toggle minimap, C = toggle coordinates"
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

# Display dispatch (runtime backend selector)
display_dispatch.o: display_dispatch.c display_dispatch.h
	$(CC) $(CFLAGS) -DUSE_SDL2 -c $< -o $@

clean:
	rm -f *.o ai/*.o $(SDL2_DISPLAY_OBJS) $(HEXMAP_OBJS) $(SPRITE_OBJS) $(HEXMAP_SPRITES_OBJS) display_dispatch.o stubs_conquer.o stubs_conqrun.o conquer conqrun test_sdl2 test_hexmap test_sprite_loader test_hexmap_sprites

.PHONY: clean all test-sdl2 test-hexmap test-sprites test-hexmap-sprites
