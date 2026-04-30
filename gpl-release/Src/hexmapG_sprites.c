/*
 * hexmapG_sprites.c - Hex Map Rendering with Sprite Integration
 * Phase 5b: Sprite Integration Implementation
 */

#include "hexmapG_sprites.h"
#include <stdio.h>
#include <string.h>

/* Elevation to sprite name mapping */
static const char *elevation_names[6] = {
    "water",    /* 0: ELE_WATER */
    "valley",   /* 1: ELE_VALLEY */
    "clear",    /* 2: ELE_CLEAR */
    "hill",     /* 3: ELE_HILL */
    "mountain", /* 4: ELE_MOUNTAIN */
    "peak"      /* 5: ELE_PEAK */
};

/* Vegetation to sprite name mapping */
static const char *vegetation_names[12] = {
    "volcano",  /* 0: VEG_VOLCANO */
    "desert",   /* 1: VEG_DESERT */
    "tundra",   /* 2: VEG_TUNDRA */
    "barren",   /* 3: VEG_BARREN */
    "lt_veg",   /* 4: VEG_LTVEG */
    "good",     /* 5: VEG_GOOD */
    "wood",     /* 6: VEG_WOOD */
    "forest",   /* 7: VEG_FOREST */
    "jungle",   /* 8: VEG_JUNGLE */
    "swamp",    /* 9: VEG_SWAMP */
    "ice",      /* 10: VEG_ICE */
    "none"      /* 11: VEG_NONE */
};

/* Default offsets per zoom level - scale proportionally */
static const sprite_offset_t default_offsets[ZOOM_LEVELS] = {
    /* ZOOM 0: 16px hex - minimal offsets */
    {0, 0,   0, 0,   0, 0,   -2, 2,   2, -2,   0, 0},
    /* ZOOM 1: 24px hex - small offsets */
    {0, 0,   0, 0,   0, -2,  -3, 3,   3, -3,   0, 0},
    /* ZOOM 2: 32px hex - standard offsets */
    {0, 0,   0, 0,   0, -4,  -6, 6,   6, -6,   0, 0},
    /* ZOOM 3: 48px hex - larger offsets */
    {0, 0,   0, 0,   0, -6,  -9, 9,   9, -9,   0, 0}
};

const char *hexmap_sprites_elevation_name(int elevation) {
    if (elevation >= 0 && elevation < 6) {
        return elevation_names[elevation];
    }
    return "clear";
}

const char *hexmap_sprites_vegetation_name(int vegetation) {
    if (vegetation >= 0 && vegetation < 12) {
        return vegetation_names[vegetation];
    }
    return "none";
}

const char *hexmap_sprites_building_name(int designation) {
    /* Simplified - just return common buildings */
    switch (designation) {
        case 0: return "farm";
        case 1: return "town";
        case 2: return "city";
        case 3: return "capital";
        case 4: return "mine";
        default: return NULL;
    }
}

int hexmap_sprites_init(hexmap_sprites_context_t *ctx, SDL_Renderer *renderer,
                        int width, int height, const char *sprite_path) {
    if (!ctx || !renderer) return -1;
    
    /* Initialize base hexmap context */
    if (hexmap_init(&ctx->base, width, height) != 0) {
        return -1;
    }
    
    /* Create sprite manager */
    ctx->sprites = sprite_manager_create(renderer, sprite_path);
    if (!ctx->sprites) {
        hexmap_shutdown(&ctx->base);
        return -1;
    }
    
    /* Load all sprites */
    sprite_load_all(ctx->sprites);
    
    /* Set default options */
    ctx->use_sprites = 1;
    ctx->show_units = 1;
    ctx->show_buildings = 1;
    ctx->show_vegetation = 1;
    
    /* Set offsets for current zoom */
    hexmap_sprites_set_offsets(ctx, ctx->base.zoom);
    
    return 0;
}

void hexmap_sprites_shutdown(hexmap_sprites_context_t *ctx) {
    if (!ctx) return;
    
    if (ctx->sprites) {
        sprite_manager_destroy(ctx->sprites);
        ctx->sprites = NULL;
    }
    
    hexmap_shutdown(&ctx->base);
}

void hexmap_sprites_set_offsets(hexmap_sprites_context_t *ctx, int zoom) {
    if (!ctx) return;
    
    if (zoom < ZOOM_MIN) zoom = ZOOM_MIN;
    if (zoom > ZOOM_MAX) zoom = ZOOM_MAX;
    
    ctx->offsets = default_offsets[zoom];
}

void hexmap_sprites_reset_offsets(hexmap_sprites_context_t *ctx) {
    if (!ctx) return;
    hexmap_sprites_set_offsets(ctx, ctx->base.zoom);
}

void hexmap_sprites_reload(hexmap_sprites_context_t *ctx) {
    if (!ctx || !ctx->sprites) return;
    sprite_reload_all(ctx->sprites);
}

/* Load specific sprite types */
int hexmap_sprites_load_terrain(hexmap_sprites_context_t *ctx, int elevation) {
    if (!ctx || !ctx->sprites) return -1;
    return sprite_load(ctx->sprites, SPRITE_CAT_TERRAIN, 
                       hexmap_sprites_elevation_name(elevation));
}

int hexmap_sprites_load_vegetation(hexmap_sprites_context_t *ctx, int vegetation) {
    if (!ctx || !ctx->sprites) return -1;
    return sprite_load(ctx->sprites, SPRITE_CAT_VEGETATION,
                       hexmap_sprites_vegetation_name(vegetation));
}

int hexmap_sprites_load_building(hexmap_sprites_context_t *ctx, int designation) {
    if (!ctx || !ctx->sprites) return -1;
    const char *name = hexmap_sprites_building_name(designation);
    if (name) {
        return sprite_load(ctx->sprites, SPRITE_CAT_BUILDINGS, name);
    }
    return -1;
}

int hexmap_sprites_load_unit(hexmap_sprites_context_t *ctx, int army_class) {
    if (!ctx || !ctx->sprites) return -1;
    /* Units not yet implemented - use placeholder */
    return 0;
}

/* Render terrain layer with sprite */
void hexmap_sprites_render_terrain(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                   hex_tile_t *tile, int px, int py, int size) {
    if (!ctx || !renderer || !tile) return;
    
    const char *terrain_name = hexmap_sprites_elevation_name(tile->elevation);
    
    if (ctx->use_sprites && ctx->sprites) {
        SDL_Texture *tex = sprite_get_texture(ctx->sprites, SPRITE_CAT_TERRAIN, terrain_name);
        if (tex) {
            /* Render sprite centered on hex */
            sprite_render(ctx->sprites, SPRITE_CAT_TERRAIN, terrain_name,
                         px + ctx->offsets.terrain_x, 
                         py + ctx->offsets.terrain_y,
                         size, size);
        } else {
            /* Fallback to colored hex */
            SDL_Color fill = hexmap_elevation_color(tile->elevation);
            SDL_Color outline = {50, 50, 50, 255};
            hexmap_draw_hex(renderer, px, py, size - 1, fill, outline);
        }
    } else {
        /* Fallback to colored hex */
        SDL_Color fill = hexmap_elevation_color(tile->elevation);
        SDL_Color outline = {50, 50, 50, 255};
        hexmap_draw_hex(renderer, px, py, size - 1, fill, outline);
    }
}

/* Render vegetation overlay */
void hexmap_sprites_render_vegetation(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                       hex_tile_t *tile, int px, int py, int size) {
    if (!ctx || !renderer || !tile) return;
    if (!ctx->show_vegetation) return;
    if (tile->vegetation == 11) return; /* VEG_NONE */
    
    const char *veg_name = hexmap_sprites_vegetation_name(tile->vegetation);
    
    if (ctx->use_sprites && ctx->sprites) {
        SDL_Texture *tex = sprite_get_texture(ctx->sprites, SPRITE_CAT_VEGETATION, veg_name);
        if (tex) {
            /* Render vegetation sprite with slight alpha blend effect */
            /* Note: SDL2 doesn't have built-in alpha modulation per-draw easily,
             * so we render at full opacity for now */
            sprite_render(ctx->sprites, SPRITE_CAT_VEGETATION, veg_name,
                         px + ctx->offsets.vegetation_x,
                         py + ctx->offsets.vegetation_y,
                         size, size);
        }
    }
}

/* Render building/designation */
void hexmap_sprites_render_building(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                     hex_tile_t *tile, int px, int py, int size) {
    if (!ctx || !renderer || !tile) return;
    if (!ctx->show_buildings) return;
    if (tile->designation == 0 && !tile->has_city) return;
    
    const char *building_name = NULL;
    
    if (tile->has_city) {
        building_name = "city";
    } else {
        building_name = hexmap_sprites_building_name(tile->designation);
    }
    
    if (building_name && ctx->use_sprites && ctx->sprites) {
        SDL_Texture *tex = sprite_get_texture(ctx->sprites, SPRITE_CAT_BUILDINGS, building_name);
        if (tex) {
            sprite_render(ctx->sprites, SPRITE_CAT_BUILDINGS, building_name,
                         px + ctx->offsets.building_x,
                         py + ctx->offsets.building_y,
                         size * 0.8, size * 0.8); /* Slightly smaller than hex */
        } else {
            /* Fallback: gold square for buildings */
            int bsize = size / 3;
            SDL_Rect brect = {px - bsize, py - bsize - 4, bsize * 2, bsize * 2};
            SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
            SDL_RenderFillRect(renderer, &brect);
        }
    }
}

/* Render units */
void hexmap_sprites_render_unit(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                 hex_tile_t *tile, int px, int py, int size) {
    if (!ctx || !renderer || !tile) return;
    if (!ctx->show_units) return;
    
    if (tile->has_army) {
        /* Ground unit - lower left offset */
        if (ctx->use_sprites && ctx->sprites) {
            /* Try to render unit sprite - using infantry as placeholder */
            sprite_render(ctx->sprites, SPRITE_CAT_UNITS, "infantry",
                         px + ctx->offsets.ground_unit_x,
                         py + ctx->offsets.ground_unit_y,
                         size * 0.6, size * 0.6);
        } else {
            /* Fallback: blue square */
            int usize = size / 4;
            SDL_Rect urect = {px + ctx->offsets.ground_unit_x - usize,
                             py + ctx->offsets.ground_unit_y - usize,
                             usize * 2, usize * 2};
            SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255);
            SDL_RenderFillRect(renderer, &urect);
        }
    }
    
    if (tile->has_navy) {
        /* Navy unit - render differently */
        if (ctx->use_sprites && ctx->sprites) {
            sprite_render(ctx->sprites, SPRITE_CAT_NAVY, "warship",
                         px + ctx->offsets.ground_unit_x,
                         py + ctx->offsets.ground_unit_y,
                         size * 0.6, size * 0.6);
        } else {
            /* Fallback: dark blue square */
            int usize = size / 4;
            SDL_Rect urect = {px + ctx->offsets.ground_unit_x - usize,
                             py + ctx->offsets.ground_unit_y - usize,
                             usize * 2, usize * 2};
            SDL_SetRenderDrawColor(renderer, 0, 0, 150, 255);
            SDL_RenderFillRect(renderer, &urect);
        }
    }
}

/* Render a single tile with all layers */
int hexmap_sprites_render_tile(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx,
                                int gx, int gy, int px, int py) {
    if (!ctx || !renderer) return -1;
    
    int idx = gy * ctx->base.map_width + gx;
    if (idx < 0 || idx >= ctx->base.map_width * ctx->base.map_height) {
        return -1;
    }
    
    hex_tile_t *tile = &ctx->base.tile_cache[idx];
    const zoom_config_t *cfg = &zoom_configs[ctx->base.zoom];
    int size = cfg->hex_size;
    
    /* Layer 1: Terrain (base) */
    hexmap_sprites_render_terrain(renderer, ctx, tile, px, py, size);
    
    /* Layer 2: Vegetation (overlay) */
    hexmap_sprites_render_vegetation(renderer, ctx, tile, px, py, size);
    
    /* Layer 3: Building/Designation */
    hexmap_sprites_render_building(renderer, ctx, tile, px, py, size);
    
    /* Layer 4: Units */
    hexmap_sprites_render_unit(renderer, ctx, tile, px, py, size);
    
    return 0;
}

/* Main render function */
int hexmap_sprites_render(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx) {
    if (!ctx || !renderer) return -1;
    
    /* Clear background */
    SDL_SetRenderDrawColor(renderer, 20, 40, 20, 255);
    SDL_RenderClear(renderer);
    
    const zoom_config_t *cfg = &zoom_configs[ctx->base.zoom];
    
    /* Calculate visible range */
    int screen_w, screen_h;
    SDL_GetRendererOutputSize(renderer, &screen_w, &screen_h);
    
    int cols_visible = screen_w / cfg->xshift + 2;
    int rows_visible = screen_h / cfg->yshift + 2;
    
    int start_x = ctx->base.view_x;
    int start_y = ctx->base.view_y;
    int end_x = start_x + cols_visible;
    int end_y = start_y + rows_visible;
    
    /* Clamp to map bounds */
    if (end_x > ctx->base.map_width) end_x = ctx->base.map_width;
    if (end_y > ctx->base.map_height) end_y = ctx->base.map_height;
    
    /* Render visible tiles */
    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int px, py;
            hexmap_grid_to_pixel(&ctx->base, x, y, &px, &py);
            hexmap_sprites_render_tile(renderer, ctx, x, y, px, py);
        }
    }
    
    /* Render cursor on top */
    hexmap_render_cursor(renderer, &ctx->base);
    
    return 0;
}
