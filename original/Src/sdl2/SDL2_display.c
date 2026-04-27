/* SDL2 Display Layer for Original Conquer V5 */

#include "SDL2_display.h"
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_WIDTH  800
#define DEFAULT_HEIGHT 600
#define DEFAULT_TITLE  "Conquer V5 (SDL2 - Original)"

static sdl2_context_t ctx = {0};

/* Initialize SDL2 display context.
 * Returns 0 on success, -1 on failure.
 */
int SDL2_display_init(sdl2_context_t *ctx, int width, int height, const char *title)
{
    int ret = 0;

    if (!ctx) {
        printf("SDL2_display_init: NULL context pointer\n");
        return -1;
    }

    if (!title) {
        title = DEFAULT_TITLE;
    }

    /* Initialize SDL2 */
    if (SDL_Init(0 | 2 | 4) < 0) {
        printf("SDL2_display_init: SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Create renderer with OpenGL ES 2.0 support */
    ctx->renderer = SDL_CreateRenderer(
        NULL,
        0,
        SDL_RENDERER_ACCELERATED |
        SDL_RENDERER_PRESENTVSYNC |
        SDL_RENDERER_SOFTFLOAT
    );

    if (!ctx->renderer) {
        printf("SDL2_display_init: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    /* Set renderer output size */
    SDL_RenderSetLogicalSize(ctx->renderer, width, height);
    SDL_RenderSetViewportSize(ctx->renderer, width, height);
    SDL_RenderClear(ctx->renderer);

    /* Create window */
    ctx->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!ctx->window) {
        printf("SDL2_display_init: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ctx->renderer);
        SDL_Quit();
        return -1;
    }

    ctx->screen_width = width;
    ctx->screen_height = height;
    ctx->is_initialized = 1;

    SDL_RenderSetLogicalSize(ctx->renderer, width, height);
    SDL_RenderSetViewportSize(ctx->renderer, width, height);
    SDL_RenderClear(ctx->renderer);

    printf("SDL2_display_init: Successfully initialized at %dx%d\n", width, height);
    printf("SDL2_display_init: Title: %s\n", title);

    return ret;
}

/* Shutdown SDL2 display context.
 * Returns 0 on success, -1 on failure.
 */
void SDL2_display_shutdown(sdl2_context_t *ctx)
{
    if (!ctx) {
        printf("SDL2_display_shutdown: NULL context pointer\n");
        return;
    }

    if (ctx->is_initialized) {
        printf("SDL2_display_shutdown: Cleaning up SDL2 resources...\n");

        if (ctx->map_texture) {
            SDL_DestroyTexture(ctx->map_texture);
            ctx->map_texture = NULL;
        }
        if (ctx->units_texture) {
            SDL_DestroyTexture(ctx->units_texture);
            ctx->units_texture = NULL;
        }
        if (ctx->ui_texture) {
            SDL_DestroyTexture(ctx->ui_texture);
            ctx->ui_texture = NULL;
        }
        if (ctx->sector_texture) {
            SDL_DestroyTexture(ctx->sector_texture);
            ctx->sector_texture = NULL;
        }
        if (ctx->unit_texture) {
            SDL_DestroyTexture(ctx->unit_texture);
            ctx->unit_texture = NULL;
        }
        if (ctx->renderer) {
            SDL_DestroyRenderer(ctx->renderer);
            ctx->renderer = NULL;
        }
        if (ctx->window) {
            SDL_DestroyWindow(ctx->window);
            ctx->window = NULL;
        }
        if (ctx->font) {
            TTF_CloseFont(ctx->font);
            ctx->font = NULL;
        }

        printf("SDL2_display_shutdown: SDL2 resources cleaned up.\n");
    }
    ctx->is_initialized = 0;
}

/* Load TTF font (stub implementation).
 * Returns 0 on success, -1 on failure.
 */
int SDL2_display_load_font(sdl2_context_t *ctx, const char *font_path, int size)
{
    if (!ctx) {
        printf("SDL2_display_load_font: NULL context pointer\n");
        return -1;
    }

    if (!font_path) {
        printf("SDL2_display_load_font: NULL font_path provided\n");
        return -1;
    }

    ctx->font = TTF_OpenFont(font_path, size);

    if (!ctx->font) {
        printf("SDL2_display_load_font: TTF_OpenFont failed: %s\n", TTF_GetError());
        return -1;
    }

    printf("SDL2_display_load_font: Successfully loaded font: %s\n", font_path);

    return 0;
}

/* Render map (stub implementation).
 * Simple colored rectangle for now.
 * Returns 0 on success, -1 on failure.
 */
int SDL2_display_render_map(sdl2_context_t *ctx, int xoffset, int yoffset)
{
    if (!ctx) {
        printf("SDL2_display_render_map: NULL context pointer\n");
        return -1;
    }

    if (!ctx->is_initialized) {
        printf("SDL2_display_render_map: Not initialized\n");
        return -1;
    }

    printf("SDL2_display_render_map: Rendering map at %dx%d offset (%d, %d)\n",
           ctx->screen_width, ctx->screen_height, xoffset, yoffset);

    SDL_Rect fillRect = {
        0, 0, ctx->screen_width, ctx->screen_height
    };
    SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 0, 255); /* Green */
    SDL_RenderFillRect(ctx->renderer, &fillRect);

    return 0;
}

/* Render sector (stub implementation).
 * Simple colored rectangle for now.
 * Returns 0 on success, -1 on failure.
 */
int SDL2_display_render_sector(sdl2_context_t *ctx, int x, int y, int sector_type)
{
    if (!ctx) {
        printf("SDL2_display_render_sector: NULL context pointer\n");
        return -1;
    }

    if (!ctx->is_initialized) {
        printf("SDL2_display_render_sector: Not initialized\n");
        return -1;
    }

    printf("SDL2_display_render_sector: Rendering sector at (%d, %d), type: %d\n",
           x, y, sector_type);

    SDL_Rect fillRect = {
        x, y, 100, 100
    };
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 0, 255); /* Yellow */
    SDL_RenderFillRect(ctx->renderer, &fillRect);

    return 0;
}

/* Render unit (stub implementation).
 * Simple colored rectangle for now.
 * Returns 0 on success, -1 on failure.
 */
int SDL2_display_render_unit(sdl2_context_t *ctx, int x, int y, int unit_type)
{
    if (!ctx) {
        printf("SDL2_display_render_unit: NULL context pointer\n");
        return -1;
    }

    if (!ctx->is_initialized) {
        printf("SDL2_display_render_unit: Not initialized\n");
        return -1;
    }

    printf("SDL2_display_render_unit: Rendering unit at (%d, %d), type: %d\n",
           x, y, unit_type);

    SDL_Rect fillRect = {
        x, y, 20, 20
    };
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 255, 255); /* Blue */
    SDL_RenderFillRect(ctx->renderer, &fillRect);

    return 0;
}

/* Render text (stub implementation).
 * Simple colored rectangle with text.
 * Returns 0 on success, -1 on failure.
 */
int SDL2_display_render_text(sdl2_context_t *ctx, int x, int y, const char *text, SDL_Color color)
{
    if (!ctx) {
        printf("SDL2_display_render_text: NULL context pointer\n");
        return -1;
    }

    if (!ctx->is_initialized) {
        printf("SDL2_display_render_text: Not initialized\n");
        return -1;
    }

    printf("SDL2_display_render_text: Rendering text '%s' at (%d, %d), color: %d, %d, %d, %d\n",
           text ? text : "(null)", x, y, color.r, color.g, color.b, color.a);

    SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, color.a);
    SDL_Rect textRect = {
        x, y, 100, 20
    };
    SDL_RenderFillRect(ctx->renderer, &textRect);

    return 0;
}

/* Present (swap buffers).
 * Returns 0 on success, -1 on failure.
 */
int SDL2_display_present(sdl2_context_t *ctx)
{
    if (!ctx) {
        printf("SDL2_display_present: NULL context pointer\n");
        return -1;
    }

    if (!ctx->is_initialized) {
        printf("SDL2_display_present: Not initialized\n");
        return -1;
    }

    SDL_RenderPresent(ctx->renderer);
    printf("SDL2_display_present: Buffer swapped.\n");

    return 0;
}

/* Handle events (key/quit).
 * Returns 0 on success, -1 on failure.
 * Sets *quit = 1 if user presses Escape or closes window.
 */
int SDL2_display_handle_events(sdl2_context_t *ctx, int *quit)
{
    if (!ctx) {
        printf("SDL2_display_handle_events: NULL context pointer\n");
        return -1;
    }

    if (!ctx->is_initialized) {
        printf("SDL2_display_handle_events: Not initialized\n");
        return -1;
    }

    if (!quit) {
        printf("SDL2_display_handle_events: NULL quit pointer\n");
        return -1;
    }

    *quit = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event) > 0) {
        if (event.type == SDL_QUIT) {
            *quit = 1;
            printf("SDL2_display_handle_events: Window closed\n");
            break;
        }
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                *quit = 1;
                printf("SDL2_display_handle_events: Escape pressed\n");
                break;
            }
        }
    }

    return 0;
}
