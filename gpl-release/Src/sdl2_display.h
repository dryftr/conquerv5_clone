#ifndef SDL2_DISPLAY_H
#define SDL2_DISPLAY_H "

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/* SDL2 display context */
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *map_texture;
    SDL_Texture *units_texture;
    TTF_Font *font;
    int screen_width;
    int screen_height;
    int is_initialized;
} sdl2_context_t;

/* Color definitions (matching original curses colors) */
#define COLOR_BLACK    (SDL_Color){0, 0, 0, 255}
#define COLOR_RED      (SDL_Color){255, 0, 0, 255}
#define COLOR_GREEN    (SDL_Color){0, 255, 0, 255}
#define COLOR_YELLOW   (SDL_Color){255, 255, 0, 255}
#define COLOR_BLUE     (SDL_Color){0, 0, 255, 255}
#define COLOR_MAGENTA (SDL_Color){255, 0, 255, 255}
#define COLOR_CYAN     (SDL_Color){0, 255, 255, 255}
#define COLOR_WHITE    (SDL_Color){255, 255, 255, 255}

/* Function declarations */
int sdl2_init(sdl2_context_t *ctx, int width, int height, const char *title);
void sdl2_shutdown(sdl2_context_t *ctx);
int sdl2_load_font(sdl2_context_t *ctx, const char *font_path, int size);
int sdl2_render_map(sdl2_context_t *ctx, int xoffset, int yoffset);
int sdl2_render_sector(sdl2_context_t *ctx, int x, int y, int sector_type);
int sdl2_render_unit(sdl2_context_t *ctx, int x, int y, int unit_type);
int sdl2_render_text(sdl2_context_t *ctx, int x, int y, const char *text, SDL_Color color);
int sdl2_present(sdl2_context_t *ctx);
int sdl2_handle_events(sdl2_context_t *ctx, int *quit);

#endif /* SDL2_DISPLAY_H */
