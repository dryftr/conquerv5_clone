/*
 * hexmapG_sdl2.h - SDL2 Hex Map Rendering for Conquer V5
 * Phase 4c: Hexagonal Tile Rendering System
 *
 * This replaces the curses-based hexmapG.c rendering with SDL2
 * hardware-accelerated hexagonal tile rendering.
 */

#ifndef HEXMAPG_SDL2_H
#define HEXMAPG_SDL2_H

#include "sdl2_display.h"
#include <SDL2/SDL.h>

/* Hex grid constants */
#define HEX_SIZE_DEFAULT    32
#define HEX_WIDTH_RATIO     0.866025f  /* sqrt(3)/2 for hex width */
#define HEX_HEIGHT_RATIO    1.0f

/* Display modes (matching original) */
#define DMODE_TERRAIN       0
#define DMODE_ELEVATION     1
#define DMODE_VEGETATION    2
#define DMODE_OWNERSHIP     3
#define DMODE_NUMBER        4

/* Hex position types */
#define HXPOS_CENTER        0
#define HXPOS_TOP           1
#define HXPOS_TOP_RIGHT     2
#define HXPOS_BOTTOM_RIGHT  3
#define HXPOS_BOTTOM        4
#define HXPOS_BOTTOM_LEFT   5
#define HXPOS_TOP_LEFT      6
#define HXPOS_NUMBER        7

/* Zoom levels */
#define ZOOM_MIN            0
#define ZOOM_MAX            3
#define ZOOM_LEVELS         4

/* Hex tile data */
typedef struct {
    int elevation;      /* 0-5: water, valley, clear, hill, mountain, peak */
    int vegetation;     /* 0-11: volcano, desert, tundra, etc. */
    int designation;    /* Major/minor designation */
    int owner;          /* Nation owner (-1 = none) */
    int has_army;       /* Army present */
    int has_navy;       /* Navy present */
    int has_city;       /* City present */
    int highlight;      /* Cursor/focus highlight */
} hex_tile_t;

/* Screen configuration per zoom level */
typedef struct {
    int hex_size;       /* Pixel size of hex */
    int xshift;         /* Horizontal shift between columns */
    int yshift;         /* Vertical shift between rows */
    int xsize;          /* Width of hex in pixels */
    int ysize;          /* Height of hex in pixels */
    int oddlift;        /* Odd column vertical offset */
    int has_border;     /* Border offset */
    int focus;          /* Focus position offset */
} zoom_config_t;

/* Hex map context */
typedef struct {
    int map_width;          /* Width in sectors */
    int map_height;         /* Height in sectors */
    int view_x;             /* View offset X */
    int view_y;             /* View offset Y */
    int cursor_x;           /* Cursor position X */
    int cursor_y;           /* Cursor position Y */
    int zoom;               /* Current zoom level */
    int display_mode;       /* Current display mode */
    SDL_Texture *hex_texture;       /* Cached hex tiles */
    SDL_Texture *unit_texture;      /* Unit sprites */
    SDL_Texture *city_texture;      /* City sprites */
    hex_tile_t *tile_cache;         /* Cached tile data */
    int cache_dirty;                /* Cache needs refresh */
} hexmap_context_t;

/* Function declarations */

/* Initialization */
int hexmap_init(hexmap_context_t *ctx, int width, int height);
void hexmap_shutdown(hexmap_context_t *ctx);

/* Rendering */
int hexmap_render(SDL_Renderer *renderer, hexmap_context_t *ctx);
int hexmap_render_tile(SDL_Renderer *renderer, hexmap_context_t *ctx, 
                       int sx, int sy, int px, int py);
int hexmap_render_cursor(SDL_Renderer *renderer, hexmap_context_t *ctx);

/* Coordinate conversion */
void hexmap_pixel_to_grid(hexmap_context_t *ctx, int px, int py, 
                          int *gx, int *gy);
void hexmap_grid_to_pixel(hexmap_context_t *ctx, int gx, int gy, 
                          int *px, int *py);

/* View control */
void hexmap_set_view(hexmap_context_t *ctx, int x, int y);
void hexmap_move_view(hexmap_context_t *ctx, int dx, int dy);
void hexmap_center_on(hexmap_context_t *ctx, int x, int y);
void hexmap_set_zoom(hexmap_context_t *ctx, int zoom);

/* Cursor */
void hexmap_set_cursor(hexmap_context_t *ctx, int x, int y);
void hexmap_move_cursor(hexmap_context_t *ctx, int dx, int dy);

/* Display mode */
void hexmap_set_display_mode(hexmap_context_t *ctx, int mode);
const char *hexmap_mode_name(int mode);

/* Helper: Draw a single hexagon */
void hexmap_draw_hex(SDL_Renderer *renderer, int cx, int cy, int size, 
                     SDL_Color fill, SDL_Color outline);

/* Get elevation/vegetation color */
SDL_Color hexmap_elevation_color(int elevation);
SDL_Color hexmap_vegetation_color(int vegetation);
SDL_Color hexmap_owner_color(int owner);

/* Extern data tables (for sprite test / integration) */
extern const zoom_config_t zoom_configs[ZOOM_LEVELS];
extern const SDL_Color elevation_colors[6];
extern const SDL_Color vegetation_colors[12];
extern const SDL_Color owner_colors[8];
extern const SDL_Color cursor_color;
extern const SDL_Color cursor_outline;

#endif /* HEXMAPG_SDL2_H */
