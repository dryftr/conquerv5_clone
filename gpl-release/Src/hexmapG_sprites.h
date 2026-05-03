/*
 * hexmapG_sprites.h - Hex Map Rendering with Sprite Integration
 * Phase 5b: Sprite Integration into Game Rendering
 *
 * Extends hexmapG_sdl2 with sprite-based rendering for terrain,
 * units, buildings, and UI elements.
 */

#ifndef HEXMAPG_SPRITES_H
#define HEXMAPG_SPRITES_H

#include "hexmapG_sdl2.h"
#include "sprite_loader.h"

/* Layer offsets for sprite rendering (relative to hex center) */
typedef struct {
    int terrain_x;      /* Terrain base layer */
    int terrain_y;
    int vegetation_x;   /* Vegetation overlay */
    int vegetation_y;
    int building_x;     /* Designation/building */
    int building_y;
    int ground_unit_x;  /* Ground units (lower-left) */
    int ground_unit_y;
    int air_unit_x;     /* Air/flying units (upper-right) */
    int air_unit_y;
    int ui_x;           /* UI/cursor */
    int ui_y;
} sprite_offset_t;

/* Zoom transition state */
typedef struct {
    int target_zoom;                /* Zoom level we're animating to */
    int start_zoom;                 /* Zoom level we started from */
    float progress;                 /* 0.0 to 1.0 */
    Uint32 start_time;              /* Animation start timestamp */
    int is_zooming;                 /* Animation in progress */
    float current_hex_size;         /* Interpolated hex size */
    float current_xshift;           /* Interpolated horizontal shift */
    float current_yshift;           /* Interpolated vertical shift */
} zoom_transition_t;

/* Minimap state */
typedef struct {
    int enabled;                    /* Minimap visible */
    int x;                          /* Screen X position */
    int y;                          /* Screen Y position */
    int width;                      /* Minimap width in pixels */
    int height;                     /* Minimap height in pixels */
    SDL_Texture *texture;           /* Cached minimap texture */
    int needs_update;               /* Texture needs refresh */
} minimap_t;

/* Coordinate overlay state */
typedef struct {
    int enabled;                    /* Show coordinates */
    int show_cube;                  /* Show cube coordinates (q,r,s) vs axial (x,y) */
    SDL_Color text_color;           /* Text color */
    int font_size;                  /* Text size in pixels */
} coord_overlay_t;

/* Extended hexmap context with sprite support */
typedef struct {
    hexmap_context_t base;          /* Base hexmap context */
    sprite_manager_t *sprites;      /* Sprite manager */
    sprite_offset_t offsets;        /* Layer offsets */
    int use_sprites;                /* Enable/disable sprite rendering */
    int show_units;                 /* Show unit sprites */
    int show_buildings;             /* Show building sprites */
    int show_vegetation;            /* Show vegetation overlay */
    zoom_transition_t zoom_trans;   /* Smooth zoom transition state */
    minimap_t minimap;              /* Minimap display */
    coord_overlay_t coords;         /* Coordinate overlay */
} hexmap_sprites_context_t;

/* Function declarations */

/* Initialization */
int hexmap_sprites_init(hexmap_sprites_context_t *ctx, SDL_Renderer *renderer,
                        int width, int height, const char *sprite_path);
void hexmap_sprites_shutdown(hexmap_sprites_context_t *ctx);

/* Sprite loading */
int hexmap_sprites_load_terrain(hexmap_sprites_context_t *ctx, int elevation);
int hexmap_sprites_load_vegetation(hexmap_sprites_context_t *ctx, int vegetation);
int hexmap_sprites_load_building(hexmap_sprites_context_t *ctx, int designation);
int hexmap_sprites_load_unit(hexmap_sprites_context_t *ctx, int army_class);
void hexmap_sprites_reload(hexmap_sprites_context_t *ctx);

/* Rendering with sprites */
int hexmap_sprites_render(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx);
int hexmap_sprites_render_tile(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                int sx, int sy, int px, int py);

/* Rendering layers */
void hexmap_sprites_render_terrain(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                   hex_tile_t *tile, int px, int py, int size);
void hexmap_sprites_render_vegetation(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                       hex_tile_t *tile, int px, int py, int size);
void hexmap_sprites_render_building(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                     hex_tile_t *tile, int px, int py, int size);
void hexmap_sprites_render_unit(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                 hex_tile_t *tile, int px, int py, int size);

/* Offset configuration */
void hexmap_sprites_set_offsets(hexmap_sprites_context_t *ctx, int zoom);
void hexmap_sprites_reset_offsets(hexmap_sprites_context_t *ctx);

/* Zoom transitions */
void hexmap_sprites_set_zoom_smooth(hexmap_sprites_context_t *ctx, int target_zoom);
void hexmap_sprites_update_zoom_transition(hexmap_sprites_context_t *ctx);
int hexmap_sprites_get_current_zoom_size(hexmap_sprites_context_t *ctx);
float hexmap_sprites_get_current_zoom_xshift(hexmap_sprites_context_t *ctx);
float hexmap_sprites_get_current_zoom_yshift(hexmap_sprites_context_t *ctx);

/* Minimap */
void hexmap_sprites_minimap_init(hexmap_sprites_context_t *ctx, SDL_Renderer *renderer,
                                  int x, int y, int width, int height);
void hexmap_sprites_minimap_toggle(hexmap_sprites_context_t *ctx);
void hexmap_sprites_minimap_render(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx);
void hexmap_sprites_minimap_update(hexmap_sprites_context_t *ctx, SDL_Renderer *renderer);

/* Coordinate overlay */
void hexmap_sprites_coords_toggle(hexmap_sprites_context_t *ctx);
void hexmap_sprites_coords_set_mode(hexmap_sprites_context_t *ctx, int cube_mode);
void hexmap_sprites_coords_render(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx);

/* Utility */
const char *hexmap_sprites_elevation_name(int elevation);
const char *hexmap_sprites_vegetation_name(int vegetation);
const char *hexmap_sprites_building_name(int designation);

#endif /* HEXMAPG_SPRITES_H */
