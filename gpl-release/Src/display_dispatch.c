/*
 * display_dispatch.c - Runtime display backend selector implementation
 *
 * Selects between ncurses and SDL2 display backends at runtime.
 * Priority: command line (-m) > environment (CONQUER_DISPLAY) > config > default
 *
 * Public repo (gpl-release) feature only. conquer_rebirth uses compile-time #ifdef.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

/* Conquer headers (need DISPLAY_STRUCT, SCREEN_STRUCT, etc.) */
#include "dataX.h"
#include "fileX.h"
#include "displayG.h"
#include "displayG_sdl2.h"
#include "sdl2_display.h"
#include "display_dispatch.h"

/* Global display ops — set during init, used throughout game code */
display_ops_t display;

/* Current backend */
static display_backend_t current_backend = DISPLAY_CURSES;

/* ---- Curses backend wrappers ---- */

static int curses_init(const char *progname) {
    cq_init((char *)progname);
    return 0;
}

static void curses_shutdown(void) {
    cq_bye(0);
}

static void curses_refresh(void) {
    refresh();
}

static void curses_clear(void) {
    clearok(curscr, TRUE);
}

static void curses_makemap(void) {
    extern void makemap(void);
    makemap();
}

static void curses_show_sect(int x, int y, int a, int b, int c) {
    extern void show_sect(int x, int y, int a, int b, int c);
    show_sect(x, y, a, b, c);
}

static void curses_show_cursor(void) {
    extern void show_cursor(void);
    show_cursor();
}

static void curses_reset(void) {
    cq_reset();
}

/* ---- Curses ops table ---- */
static display_ops_t curses_ops = {
    .init        = curses_init,
    .shutdown    = curses_shutdown,
    .refresh     = curses_refresh,
    .clear       = curses_clear,
    .makemap     = curses_makemap,
    .show_sect   = curses_show_sect,
    .show_cursor = curses_show_cursor,
    .reset       = curses_reset,
};

/* ---- SDL2 backend wrappers ---- */

static int sdl2_init_wrapper(const char *progname) {
    return sdl2_display_init_with_opts(progname);
}

static void sdl2_shutdown_wrapper(void) {
    sdl2_display_shutdown();
}

/* ---- SDL2 ops table ---- */
static display_ops_t sdl2_ops = {
    .init        = sdl2_init_wrapper,
    .shutdown    = sdl2_shutdown_wrapper,
    .refresh     = sdl2_refresh,
    .clear       = sdl2_clear,
    .makemap     = sdl2_makemap,
    .show_sect   = sdl2_show_sect,
    .show_cursor = sdl2_show_cursor,
    .reset       = sdl2_refresh,
};

/* ---- Public API ---- */

void display_set_backend(display_backend_t backend) {
    current_backend = backend;
}

display_backend_t display_get_backend(void) {
    return current_backend;
}

display_backend_t display_backend_from_string(const char *str) {
    if (!str) return DISPLAY_AUTO;

    if (strcasecmp(str, "curses") == 0 ||
        strcasecmp(str, "ncurses") == 0 ||
        strcasecmp(str, "terminal") == 0 ||
        strcasecmp(str, "tty") == 0) {
        return DISPLAY_CURSES;
    }
    if (strcasecmp(str, "sdl2") == 0 ||
        strcasecmp(str, "sdl") == 0 ||
        strcasecmp(str, "graphical") == 0 ||
        strcasecmp(str, "gui") == 0 ||
        strcasecmp(str, "graphics") == 0) {
        return DISPLAY_SDL2;
    }
    if (strcasecmp(str, "auto") == 0) {
        return DISPLAY_AUTO;
    }

    fprintf(stderr, "Unknown display backend: '%s' (use: curses, sdl2, auto)\n", str);
    return DISPLAY_AUTO;
}

int display_init(const char *progname, int argc, char **argv) {
    display_backend_t backend = DISPLAY_AUTO;
    int backend_specified = 0;

    /* Priority 1: Command line -m flag */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            backend = display_backend_from_string(argv[i + 1]);
            backend_specified = 1;
            break;
        }
    }

    /* Priority 2: Environment variable CONQUER_DISPLAY */
    if (!backend_specified) {
        const char *env_display = getenv("CONQUER_DISPLAY");
        if (env_display) {
            backend = display_backend_from_string(env_display);
            backend_specified = 1;
        }
    }

    /* Priority 3: Config file — TODO */

    /* Priority 4: Default (AUTO = try SDL2 first, fall back to curses) */

    /* Try to initialize the selected backend */
    if (backend == DISPLAY_AUTO || backend == DISPLAY_SDL2) {
        if (sdl2_init_wrapper(progname) == 0) {
            display = sdl2_ops;
            current_backend = DISPLAY_SDL2;
            fprintf(stderr, "Conquer: SDL2 display initialized\n");
            return 0;
        }
        /* SDL2 failed or not available */
        if (backend == DISPLAY_SDL2) {
            fprintf(stderr, "Conquer: SDL2 display failed, falling back to curses\n");
        }
    }

    /* Fall back to curses (always available) */
    if (backend == DISPLAY_AUTO || backend == DISPLAY_CURSES) {
        curses_init(progname);
        display = curses_ops;
        current_backend = DISPLAY_CURSES;
        return 0;
    }

    /* Shouldn't reach here */
    fprintf(stderr, "Conquer: No display backend available\n");
    return -1;
}