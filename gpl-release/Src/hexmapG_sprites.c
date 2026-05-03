/*
 * hexmapG_sprites.c - Hex Map Rendering with Sprite Integration
 * Phase 5b/5c: Sprite Integration + Map Polish
 */

#include "hexmapG_sprites.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

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
    
    /* Initialize zoom transition */
    ctx->zoom_trans.target_zoom = ctx->base.zoom;
    ctx->zoom_trans.start_zoom = ctx->base.zoom;
    ctx->zoom_trans.progress = 1.0f;
    ctx->zoom_trans.is_zooming = 0;
    ctx->zoom_trans.current_hex_size = zoom_configs[ctx->base.zoom].hex_size;
    ctx->zoom_trans.current_xshift = zoom_configs[ctx->base.zoom].xshift;
    ctx->zoom_trans.current_yshift = zoom_configs[ctx->base.zoom].yshift;
    
    /* Initialize minimap (disabled by default) */
    ctx->minimap.enabled = 0;
    ctx->minimap.x = 0;
    ctx->minimap.y = 0;
    ctx->minimap.width = 0;
    ctx->minimap.height = 0;
    ctx->minimap.texture = NULL;
    ctx->minimap.needs_update = 0;
    
    /* Initialize coordinate overlay (disabled by default) */
    ctx->coords.enabled = 0;
    ctx->coords.show_cube = 0;
    ctx->coords.text_color = (SDL_Color){255, 255, 255, 200};
    ctx->coords.font_size = 10;
    
    /* Set offsets for current zoom */
    hexmap_sprites_set_offsets(ctx, ctx->base.zoom);
    
    return 0;
}

void hexmap_sprites_shutdown(hexmap_sprites_context_t *ctx) {
    if (!ctx) return;
    
    /* Cleanup minimap texture */
    if (ctx->minimap.texture) {
        SDL_DestroyTexture(ctx->minimap.texture);
        ctx->minimap.texture = NULL;
    }
    
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
    
    /* Update zoom transition */
    hexmap_sprites_update_zoom_transition(ctx);
    
    /* Clear background */
    SDL_SetRenderDrawColor(renderer, 20, 40, 20, 255);
    SDL_RenderClear(renderer);
    
    /* Get current zoom config (may be interpolated during transition) */
    float xshift = hexmap_sprites_get_current_zoom_xshift(ctx);
    float yshift = hexmap_sprites_get_current_zoom_yshift(ctx);
    
    /* Calculate visible range */
    int screen_w, screen_h;
    SDL_GetRendererOutputSize(renderer, &screen_w, &screen_h);
    
    int cols_visible = screen_w / xshift + 2;
    int rows_visible = screen_h / yshift + 2;
    
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
    
    /* Render coordinate overlay */
    if (ctx->coords.enabled) {
        hexmap_sprites_coords_render(renderer, ctx);
    }
    
    /* Render minimap */
    if (ctx->minimap.enabled) {
        hexmap_sprites_minimap_render(renderer, ctx);
    }
    
    return 0;
}

/* ============================================================================
 * Phase 5c: Zoom Transitions
 * ============================================================================ */

void hexmap_sprites_set_zoom_smooth(hexmap_sprites_context_t *ctx, int target_zoom) {
    if (!ctx) return;
    
    /* Clamp target zoom */
    if (target_zoom < ZOOM_MIN) target_zoom = ZOOM_MIN;
    if (target_zoom > ZOOM_MAX) target_zoom = ZOOM_MAX;
    
    /* Already at target or transitioning to same target */
    if (ctx->base.zoom == target_zoom && !ctx->zoom_trans.is_zooming) {
        return;
    }
    
    /* Start new transition */
    ctx->zoom_trans.start_zoom = ctx->base.zoom;
    ctx->zoom_trans.target_zoom = target_zoom;
    ctx->zoom_trans.progress = 0.0f;
    ctx->zoom_trans.is_zooming = 1;
    ctx->zoom_trans.start_time = SDL_GetTicks();
    
    /* Update base zoom immediately (logic uses this) */
    ctx->base.zoom = target_zoom;
    
    /* Update offsets for target zoom */
    hexmap_sprites_set_offsets(ctx, target_zoom);
}

void hexmap_sprites_update_zoom_transition(hexmap_sprites_context_t *ctx) {
    if (!ctx || !ctx->zoom_trans.is_zooming) return;
    
    /* Animation duration: 250ms */
    const Uint32 DURATION_MS = 250;
    
    Uint32 now = SDL_GetTicks();
    Uint32 elapsed = now - ctx->zoom_trans.start_time;
    
    if (elapsed >= DURATION_MS) {
        /* Animation complete */
        ctx->zoom_trans.progress = 1.0f;
        ctx->zoom_trans.is_zooming = 0;
    } else {
        /* Calculate progress with easing (ease-out cubic) */
        float t = (float)elapsed / DURATION_MS;
        ctx->zoom_trans.progress = 1.0f - powf(1.0f - t, 3.0f);
    }
    
    /* Interpolate zoom parameters */
    const zoom_config_t *start_cfg = &zoom_configs[ctx->zoom_trans.start_zoom];
    const zoom_config_t *target_cfg = &zoom_configs[ctx->zoom_trans.target_zoom];
    
    float p = ctx->zoom_trans.progress;
    ctx->zoom_trans.current_hex_size = start_cfg->hex_size * (1.0f - p) + target_cfg->hex_size * p;
    ctx->zoom_trans.current_xshift = start_cfg->xshift * (1.0f - p) + target_cfg->xshift * p;
    ctx->zoom_trans.current_yshift = start_cfg->yshift * (1.0f - p) + target_cfg->yshift * p;
}

int hexmap_sprites_get_current_zoom_size(hexmap_sprites_context_t *ctx) {
    if (!ctx) return zoom_configs[2].hex_size; /* Default to medium */
    if (!ctx->zoom_trans.is_zooming) {
        return zoom_configs[ctx->base.zoom].hex_size;
    }
    return (int)(ctx->zoom_trans.current_hex_size + 0.5f);
}

float hexmap_sprites_get_current_zoom_xshift(hexmap_sprites_context_t *ctx) {
    if (!ctx) return zoom_configs[2].xshift;
    if (!ctx->zoom_trans.is_zooming) {
        return (float)zoom_configs[ctx->base.zoom].xshift;
    }
    return ctx->zoom_trans.current_xshift;
}

float hexmap_sprites_get_current_zoom_yshift(hexmap_sprites_context_t *ctx) {
    if (!ctx) return zoom_configs[2].yshift;
    if (!ctx->zoom_trans.is_zooming) {
        return (float)zoom_configs[ctx->base.zoom].yshift;
    }
    return ctx->zoom_trans.current_yshift;
}

/* ============================================================================
 * Phase 5c: Minimap
 * ============================================================================ */

void hexmap_sprites_minimap_init(hexmap_sprites_context_t *ctx, SDL_Renderer *renderer,
                                  int x, int y, int width, int height) {
    if (!ctx || !renderer) return;
    
    ctx->minimap.x = x;
    ctx->minimap.y = y;
    ctx->minimap.width = width;
    ctx->minimap.height = height;
    ctx->minimap.enabled = 1;
    ctx->minimap.needs_update = 1;
    
    /* Create texture for minimap */
    if (ctx->minimap.texture) {
        SDL_DestroyTexture(ctx->minimap.texture);
    }
    
    ctx->minimap.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                              SDL_TEXTUREACCESS_TARGET,
                                              ctx->base.map_width, ctx->base.map_height);
}

void hexmap_sprites_minimap_toggle(hexmap_sprites_context_t *ctx) {
    if (!ctx) return;
    ctx->minimap.enabled = !ctx->minimap.enabled;
    if (ctx->minimap.enabled) {
        ctx->minimap.needs_update = 1;
    }
}

void hexmap_sprites_minimap_update(hexmap_sprites_context_t *ctx, SDL_Renderer *renderer) {
    if (!ctx || !renderer || !ctx->minimap.texture) return;
    
    /* Save current render target */
    SDL_Texture *old_target = SDL_GetRenderTarget(renderer);
    
    /* Set minimap as render target */
    SDL_SetRenderTarget(renderer, ctx->minimap.texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    
    /* Draw each hex as a single pixel (or small rectangle) */
    for (int y = 0; y < ctx->base.map_height; y++) {
        for (int x = 0; x < ctx->base.map_width; x++) {
            int idx = y * ctx->base.map_width + x;
            hex_tile_t *tile = &ctx->base.tile_cache[idx];
            
            /* Use elevation color as base */
            SDL_Color color = hexmap_elevation_color(tile->elevation);
            
            /* Darken based on vegetation */
            if (tile->vegetation != 11) { /* Not VEG_NONE */
                color.r = color.r * 8 / 10;
                color.g = color.g * 8 / 10;
                color.b = color.b * 8 / 10;
            }
            
            /* Highlight owned hexes */
            if (tile->owner >= 0 && tile->owner < 8) {
                SDL_Color owner = owner_colors[tile->owner];
                color.r = (color.r + owner.r) / 2;
                color.g = (color.g + owner.g) / 2;
                color.b = (color.b + owner.b) / 2;
            }
            
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }
    
    /* Restore original render target */
    SDL_SetRenderTarget(renderer, old_target);
    
    ctx->minimap.needs_update = 0;
}

void hexmap_sprites_minimap_render(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx) {
    if (!ctx || !renderer || !ctx->minimap.enabled) return;
    
    /* Update minimap texture if needed */
    if (ctx->minimap.needs_update || !ctx->minimap.texture) {
        hexmap_sprites_minimap_update(ctx, renderer);
    }
    
    if (!ctx->minimap.texture) return;
    
    int screen_w, screen_h;
    SDL_GetRendererOutputSize(renderer, &screen_w, &screen_h);
    
    /* Default position: bottom-right corner */
    int mx = ctx->minimap.x;
    int my = ctx->minimap.y;
    int mw = ctx->minimap.width;
    int mh = ctx->minimap.height;
    
    if (mw == 0) {
        mw = 150;
        mh = 100;
        mx = screen_w - mw - 10;
        my = screen_h - mh - 10;
    }
    
    /* Draw minimap background */
    SDL_Rect bg = {mx - 2, my - 2, mw + 4, mh + 4};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 200);
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &bg);
    
    /* Render minimap texture scaled to fit */
    SDL_Rect dst = {mx, my, mw, mh};
    SDL_RenderCopy(renderer, ctx->minimap.texture, NULL, &dst);
    
    /* Draw view rectangle */
    int view_w = (int)((float)mw * screen_w / (ctx->base.map_width * zoom_configs[2].xshift));
    int view_h = (int)((float)mh * screen_h / (ctx->base.map_height * zoom_configs[2].yshift));
    int view_x = mx + (ctx->base.view_x * mw / ctx->base.map_width);
    int view_y = my + (ctx->base.view_y * mh / ctx->base.map_height);
    
    if (view_x + view_w > mx + mw) view_w = mx + mw - view_x;
    if (view_y + view_h > my + mh) view_h = my + mh - view_y;
    
    SDL_Rect view_rect = {view_x, view_y, view_w, view_h};
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 200);
    SDL_RenderDrawRect(renderer, &view_rect);
}

/* ============================================================================
 * Phase 5c: Coordinate Overlay
 * ============================================================================ */

void hexmap_sprites_coords_toggle(hexmap_sprites_context_t *ctx) {
    if (!ctx) return;
    ctx->coords.enabled = !ctx->coords.enabled;
}

void hexmap_sprites_coords_set_mode(hexmap_sprites_context_t *ctx, int cube_mode) {
    if (!ctx) return;
    ctx->coords.show_cube = cube_mode;
}

/* Simple number-to-string for coordinate display (no SDL_ttf dependency) */
static void int_to_str(int n, char *buf, int max_len) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    int neg = n < 0;
    if (neg) n = -n;
    
    int i = 0;
    while (n > 0 && i < max_len - 1) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    if (neg && i < max_len - 1) {
        buf[i++] = '-';
    }
    
    /* Reverse */
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    buf[i] = '\0';
}

/* Simple digit drawing using SDL_RenderDrawPoint (no font dependency) */
static void draw_digit(SDL_Renderer *r, int x, int y, int digit, int size, SDL_Color c) {
    /* 3x5 pixel patterns for digits 0-9 */
    static const uint16_t patterns[10] = {
        0x7B6E, /* 0: ### */ /* ### */ /* ### */
        0x4924, /* 1:  .#  */ /* ##  */ /* .#  */
        0x73CE, /* 2: ### */ /* ..# */ /* ### */
        0x79CE, /* 3: ### */ /*  .# */ /* ### */
        0x5B52, /* 4: #.# */ /* ### */ /* ..# */
        0x7A5E, /* 5: ### */ /* ##  */ /* ### */
        0x7A6E, /* 6: ### */ /* ##  */ /* ### */
        0x492E, /* 7: ### */ /* ..# */ /* .#. */
        0x7B6E, /* 8: ### */ /* ### */ /* ### */
        0x7B5E, /* 9: ### */ /* ### */ /* ### */
    };
    
    if (digit < 0 || digit > 9) return;
    
    uint16_t pat = patterns[digit];
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            int bit = 14 - (row * 3 + col);
            if ((pat >> bit) & 1) {
                SDL_Rect rect = {x + col * size, y + row * size, size, size};
                SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
                SDL_RenderFillRect(r, &rect);
            }
        }
    }
}

void hexmap_sprites_coords_render(SDL_Renderer *renderer, hexmap_sprites_context_t *ctx) {
    if (!ctx || !renderer || !ctx->coords.enabled) return;
    
    int screen_w, screen_h;
    SDL_GetRendererOutputSize(renderer, &screen_w, &screen_h);
    
    float xshift = hexmap_sprites_get_current_zoom_xshift(ctx);
    float yshift = hexmap_sprites_get_current_zoom_yshift(ctx);
    
    int cols_visible = screen_w / xshift + 2;
    int rows_visible = screen_h / yshift + 2;
    
    int start_x = ctx->base.view_x;
    int start_y = ctx->base.view_y;
    int end_x = start_x + cols_visible;
    int end_y = start_y + rows_visible;
    
    if (end_x > ctx->base.map_width) end_x = ctx->base.map_width;
    if (end_y > ctx->base.map_height) end_y = ctx->base.map_height;
    
    /* Only draw coordinates at larger zoom levels */
    int hex_size = hexmap_sprites_get_current_zoom_size(ctx);
    if (hex_size < 24) return; /* Too small to read */
    
    int digit_size = hex_size >= 32 ? 2 : 1;
    
    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int px, py;
            hexmap_grid_to_pixel(&ctx->base, x, y, &px, &py);
            
            /* Draw axial coordinates (x,y) */
            char buf[16];
            int_to_str(x, buf, sizeof(buf));
            int len = 0;
            while (buf[len]) len++;
            
            int text_x = px - len * 4 * digit_size;
            int text_y = py - 5;
            
            /* Draw x coordinate */
            for (int i = 0; buf[i]; i++) {
                draw_digit(renderer, text_x + i * 4 * digit_size, text_y, 
                          buf[i] - '0', digit_size, ctx->coords.text_color);
            }
            
            /* Draw y coordinate below */
            int_to_str(y, buf, sizeof(buf));
            len = 0;
            while (buf[len]) len++;
            text_x = px - len * 4 * digit_size;
            text_y = py + 2;
            
            for (int i = 0; buf[i]; i++) {
                draw_digit(renderer, text_x + i * 4 * digit_size, text_y,
                          buf[i] - '0', digit_size, ctx->coords.text_color);
            }
        }
    }
}
