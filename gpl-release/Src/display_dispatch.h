/*
 * display_dispatch.h - Runtime display backend selector
 *
 * Conquer V5 can switch between ncurses (terminal) and SDL2 (graphical)
 * display backends at runtime via:
 *   - Command-line flag: -G curses | -G sdl2
 *   - Config file: display_mode = curses | sdl2
 *   - Environment variable: CONQUER_DISPLAY=curses | sdl2
 *   - Default fallback: curses (always available)
 *
 * Priority: command line > environment > config file > default
 *
 * The dispatch layer replaces #ifdef USE_SDL2 blocks in mainG.c with
 * runtime function pointer dispatch. Both backends must implement the
 * same interface (display_ops_t).
 *
 * For conquer_rebirth (private repo): compile-time #ifdef USE_SDL2 remains.
 * Only the public gpl-release gets this runtime toggle.
 */

#ifndef DISPLAY_DISPATCH_H
#define DISPLAY_DISPATCH_H

/* Display backend identifiers */
typedef enum {
    DISPLAY_CURSES = 0,    /* Terminal/ncurses backend (default) */
    DISPLAY_SDL2   = 1,    /* SDL2 graphical backend */
    DISPLAY_AUTO    = 2     /* Try SDL2, fall back to curses */
} display_backend_t;

/* Display operations interface.
 * Both backends implement these functions.
 * Function pointers are set at init time based on selected backend.
 */
typedef struct {
    /* Initialize the display */
    int  (*init)(const char *progname);

    /* Shut down the display */
    void (*shutdown)(void);

    /* Refresh/redraw the screen */
    void (*refresh)(void);

    /* Clear the screen */
    void (*clear)(void);

    /* Draw the game map */
    void (*makemap)(void);

    /* Show a sector on the map */
    void (*show_sect)(int x, int y, int a, int b, int c);

    /* Show cursor/highlight */
    void (*show_cursor)(void);

    /* Reset display state */
    void (*reset)(void);

} display_ops_t;

/* Global display operations (set during init) */
extern display_ops_t display;

/* Configuration */
void display_set_backend(display_backend_t backend);
display_backend_t display_get_backend(void);

/* Parse backend from string */
display_backend_t display_backend_from_string(const char *str);

/* Initialize the selected backend.
 * Checks command line, environment, and config file.
 * Falls back to curses if requested backend is unavailable.
 * Returns 0 on success, -1 on failure.
 */
int display_init(const char *progname, int argc, char **argv);

/* Convenience macros — these replace direct curses/SDL2 calls throughout
 * the game code. Instead of cq_init() or sdl2_display_init(), use:
 *   display.init(progname);
 *   display.refresh();
 *   display.clear();
 *   etc.
 */

#endif /* DISPLAY_DISPATCH_H */