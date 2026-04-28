/* SDL2 Display Layer (Phase 4) for Conquer V5 Modernization
 *
 * This file implements the SDL2 backend for graphics rendering.
 * It replaces the original curses-based displayG.c for the modernized version.
 *
 * Build: gcc -lSDL2 -lSDL2_ttf sdl2_display.c -o sdl2_display
 */

#include "sdl2_display.h"
#include <stdio.h>
#include <stdlib.h>

/* Default SDL2 constants */
#define DEFAULT_WIDTH  800
#define DEFAULT_HEIGHT 600
#define DEFAULT_TITLE  "Conquer V5 (SDL2)"

/* SDL2 Display Context */
static sdl2_context_t sdl2_ctx = {0};

/* --- Helper: parse resolution from WxH string */
static int parse_res(const char *s, int *w, int *h) {
    if (sscanf(s, "%dx%d", w, h) == 2) return 1;
    return 0;
}

/* --- Public API (matches the header) */
int SDL2_display_init(sdl2_context_t *ctx, int width, int height, const char *title) {
    if (!ctx) {
        printf("SDL2_display_init: NULL context pointer\n");
        return -1;
    }

    if (!title) title = DEFAULT_TITLE;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL2_display_init: SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }
    /* Initialize SDL_ttf */
    if (TTF_Init() < 0) {
        printf("SDL2_display_init: TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return -1;
    }

    /* Create window */
    ctx->window = SDL_CreateWindow(title,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   width, height,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!ctx->window) {
        printf("SDL2_display_init: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    /* Create renderer */
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1,
                                       SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx->renderer) {
        printf("SDL2_display_init: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        return -1;
    }

    /* Default sizes */
    ctx->screen_width  = width;
    ctx->screen_height = height;
    ctx->is_initialized = 1;
    return 0;
}

void SDL2_display_shutdown(sdl2_context_t *ctx) {
    if (!ctx) return;
    if (!ctx->is_initialized) return;

    if (ctx->map_texture) SDL_DestroyTexture(ctx->map_texture);
    if (ctx->units_texture) SDL_DestroyTexture(ctx->units_texture);
    if (ctx->ui_texture) SDL_DestroyTexture(ctx->ui_texture);
    if (ctx->sector_texture) SDL_DestroyTexture(ctx->sector_texture);
    if (ctx->unit_texture) SDL_DestroyTexture(ctx->unit_texture);
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window) SDL_DestroyWindow(ctx->window);
    if (ctx->font) TTF_CloseFont(ctx->font);

    SDL_Quit();
    ctx->is_initialized = 0;
}

int SDL2_display_load_font(sdl2_context_t *ctx, const char *path, int size) {
    if (!ctx || !path) return -1;
    ctx->font = TTF_OpenFont(path, size);
    if (!ctx->font) {
        printf("SDL2_display_load_font: %s\n", TTF_GetError());
        return -1;
    }
    return 0;
}

int SDL2_display_render_map(sdl2_context_t *ctx, int xoffset, int yoffset) {
    if (!ctx || !ctx->is_initialized) return -1;
    /* Clear to green background */
    SDL_SetRenderDrawColor(ctx->renderer, 0, 100, 0, 255);
    SDL_Rect rect = {0, 0, ctx->screen_width, ctx->screen_height};
    SDL_RenderFillRect(ctx->renderer, &rect);
    return 0;
}

int SDL2_display_render_sector(sdl2_context_t *ctx, int x, int y, int sector_type) {
    if (!ctx || !ctx->is_initialized) return -1;
    /* Stub: yellow square for sector */
    SDL_Rect r = {x * 32, y * 32, 30, 30};
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 0, 255);
    SDL_RenderFillRect(ctx->renderer, &r);
    return 0;
}

int SDL2_display_render_unit(sdl2_context_t *ctx, int x, int y, int unit_type) {
    if (!ctx || !ctx->is_initialized) return -1;
    /* Stub: blue square for unit */
    SDL_Rect r = {x * 32 + 8, y * 32 + 8, 16, 16};
    SDL_SetRenderDrawColor(ctx->renderer, 0, 100, 255, 255);
    SDL_RenderFillRect(ctx->renderer, &r);
    return 0;
}

int SDL2_display_render_text(sdl2_context_t *ctx, int x, int y, const char *text, SDL_Color color) {
    if (!ctx || !ctx->is_initialized || !text) return -1;
    /* Stub: no font rendering yet, just log */
    printf("SDL2_display_render_text: (%d,%d) %s\n", x, y, text);
    return 0;
}

int SDL2_display_present(sdl2_context_t *ctx) {
    if (!ctx || !ctx->is_initialized) return -1;
    SDL_RenderPresent(ctx->renderer);
    return 0;
}

int SDL2_display_handle_events(sdl2_context_t *ctx, int *quit) {
    if (!ctx || !ctx->is_initialized || !quit) return -1;
    SDL_Event e;
    *quit = 0;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) *quit = 1;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) *quit = 1;
    }
    return 0;
}
