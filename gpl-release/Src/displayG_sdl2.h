#ifndef DISPLAYG_SDL2_H
#define DISPLAYG_SDL2_H

#include "sdl2_display.h"

extern int sdl2_window_width;
extern int sdl2_window_height;

void sdl2_set_resolution(int w, int h);
int sdl2_display_init_with_opts(const char *title);
void sdl2_display_shutdown(void);

// Mirrors for curses functions
void sdl2_makemap(void);
void sdl2_show_sect(int x, int y, int a, int b, int c);
void sdl2_show_cursor(void);
void sdl2_clear(void);
void sdl2_refresh(void);

#endif
