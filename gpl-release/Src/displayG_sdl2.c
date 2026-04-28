#include "displayG_sdl2.h"
#include <stdio.h>
#include <stdlib.h>

sdl2_context_t sdl2_ctx = {0};
int sdl2_inited = 0;

// Resolution preferences (set via command-line -r)
int sdl2_window_width = 0;
int sdl2_window_height = 0;

void sdl2_set_resolution(int w, int h) {
    sdl2_window_width = w;
    sdl2_window_height = h;
}

int sdl2_display_init_with_opts(const char *title) {
    int w = sdl2_window_width;
    int h = sdl2_window_height;
    
    // If not set via command line, check environment
    if (w <= 0) {
        const char *ew = getenv("SDL2_WIN_W");
        w = ew ? atoi(ew) : 800;
    }
    if (h <= 0) {
        const char *eh = getenv("SDL2_WIN_H");
        h = eh ? atoi(eh) : 600;
    }
    
    if (SDL2_display_init(&sdl2_ctx, w, h, title) != 0) {
        return -1;
    }
    sdl2_inited = 1;
    return 0;
}

void sdl2_display_shutdown(void) {
    if (sdl2_inited) {
        SDL2_display_shutdown(&sdl2_ctx);
        sdl2_inited = 0;
    }
}

void sdl2_makemap(void) {
    if (!sdl2_inited) return;
    SDL2_display_render_map(&sdl2_ctx, 0, 0);
}

void sdl2_show_sect(int x, int y, int a, int b, int c) {
    if (!sdl2_inited) return;
    SDL2_display_render_sector(&sdl2_ctx, x, y, a);
}

void sdl2_show_cursor(void) { /* Stub */ }
void sdl2_clear(void) { if (sdl2_inited) SDL_RenderClear(sdl2_ctx.renderer); }
void sdl2_refresh(void) { if (sdl2_inited) SDL_RenderPresent(sdl2_ctx.renderer); }
