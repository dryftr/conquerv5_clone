/*
 * hexmapG_sdl2.c - SDL2 Hex Map Rendering Implementation
 * Phase 4c: Hexagonal Tile Rendering System
 */

#include "hexmapG_sdl2.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Zoom level configurations */
static const zoom_config_t zoom_configs[ZOOM_LEVELS] = {
    /* ZOOM 0: Far - small hexes, many visible */
    {16,  14, 14,  16, 14,  7, 1, 0},
    /* ZOOM 1: Medium */
    {24,  21, 21,  24, 21, 10, 1, 0},
    /* ZOOM 2: Close */
    {32,  28, 28,  32, 28, 14, 1, 0},
    /* ZOOM 3: Closest - large hexes, detail view */
    {48,  42, 42,  48, 42, 21, 1, 0}
};

/* Elevation colors (matching curses approximations) */
static const SDL_Color elevation_colors[6] = {
    {0,   100, 200, 255},   /* 0: Water - Cyan-Blue */
    {144, 238, 144, 255},   /* 1: Valley - Light Green */
    {128, 128, 128, 255},   /* 2: Clear - Gray */
    {139,  69,  19, 255},   /* 3: Hill - Brown */
    {105, 105, 105, 255},   /* 4: Mountain - Dark Gray */
    {112, 128, 144, 255}    /* 5: Peak - Slate Gray */
};

/* Vegetation colors */
static const SDL_Color vegetation_colors[12] = {
    {255,  69,   0, 255},   /* 0: Volcano - Red/Orange */
    {244, 164,  96, 255},   /* 1: Desert - Sandy */
    {230, 230, 250, 255},   /* 2: Tundra - Lavender */
    { 92,  64,  51, 255},   /* 3: Barren - Dark Brown */
    {152, 251, 152, 255},   /* 4: Light Veg - Pale Green */
    { 34, 139,  34, 255},   /* 5: Good - Forest Green */
    {189, 183, 107, 255},   /* 6: Wood - Dark Yellow */
    {  0, 100,   0, 255},   /* 7: Forest - Deep Green */
    { 50, 205,  50, 255},   /* 8: Jungle - Lime */
    {  0, 128, 128, 255},   /* 9: Swamp - Teal */
    {173, 216, 230, 255},   /* 10: Ice - Ice Blue */
    {  0,   0,   0, 255}    /* 11: None - Black */
};

/* Nation owner colors (8 nations) */
static const SDL_Color owner_colors[8] = {
    {255,   0,   0, 255},   /* 0: Red */
    {  0,   0, 255, 255},   /* 1: Blue */
    {  0, 255,   0, 255},   /* 2: Green */
    {255, 255,   0, 255},   /* 3: Yellow */
    {255,   0, 255, 255},   /* 4: Magenta */
    {  0, 255, 255, 255},   /* 5: Cyan */
    {255, 165,   0, 255},   /* 6: Orange */
    {128,   0, 128, 255}    /* 7: Purple */
};

/* Cursor highlight color */
static const SDL_Color cursor_color = {255, 255, 0, 200};
static const SDL_Color cursor_outline = {255, 200, 0, 255};

int hexmap_init(hexmap_context_t *ctx, int width, int height) {
    if (!ctx) return -1;
    
    ctx->map_width = width;
    ctx->map_height = height;
    ctx->view_x = 0;
    ctx->view_y = 0;
    ctx->cursor_x = width / 2;
    ctx->cursor_y = height / 2;
    ctx->zoom = 2;  /* Start at medium zoom */
    ctx->display_mode = DMODE_TERRAIN;
    ctx->hex_texture = NULL;
    ctx->unit_texture = NULL;
    ctx->city_texture = NULL;
    ctx->tile_cache = NULL;
    ctx->cache_dirty = 1;
    
    /* Allocate tile cache */
    ctx->tile_cache = calloc(width * height, sizeof(hex_tile_t));
    if (!ctx->tile_cache) {
        fprintf(stderr, "hexmap_init: Failed to allocate tile cache\n");
        return -1;
    }
    
    return 0;
}

void hexmap_shutdown(hexmap_context_t *ctx) {
    if (!ctx) return;
    
    if (ctx->hex_texture) {
        SDL_DestroyTexture(ctx->hex_texture);
        ctx->hex_texture = NULL;
    }
    if (ctx->unit_texture) {
        SDL_DestroyTexture(ctx->unit_texture);
        ctx->unit_texture = NULL;
    }
    if (ctx->city_texture) {
        SDL_DestroyTexture(ctx->city_texture);
        ctx->city_texture = NULL;
    }
    if (ctx->tile_cache) {
        free(ctx->tile_cache);
        ctx->tile_cache = NULL;
    }
}

/* Calculate hexagon vertices */
static void calc_hex_vertices(int cx, int cy, int size, SDL_Point *points) {
    float angle_deg;
    float angle_rad;
    
    for (int i = 0; i < 6; i++) {
        angle_deg = 60 * i - 30;  /* Start at 30 degrees for pointy-top */
        angle_rad = angle_deg * M_PI / 180.0f;
        points[i].x = cx + size * cosf(angle_rad);
        points[i].y = cy + size * sinf(angle_rad);
    }
}

void hexmap_draw_hex(SDL_Renderer *renderer, int cx, int cy, int size,
                     SDL_Color fill, SDL_Color outline) {
    SDL_Point points[7];  /* 6 vertices + 1 to close */
    
    calc_hex_vertices(cx, cy, size, points);
    points[6] = points[0];  /* Close the polygon */
    
    /* Fill */
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    
    /* Simple fill: draw lines from center to edges and fill */
    /* For now, use a filled polygon approximation */
    /* Note: SDL doesn't have filled polygon, so we use a solid rect as fallback */
    SDL_Rect fill_rect = {cx - size/2, cy - size/2, size, size};
    SDL_RenderFillRect(renderer, &fill_rect);
    
    /* Outline */
    SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, outline.a);
    SDL_RenderDrawLines(renderer, points, 7);
}

SDL_Color hexmap_elevation_color(int elevation) {
    if (elevation < 0 || elevation >= 6) {
        elevation = 2;  /* Default to clear */
    }
    return elevation_colors[elevation];
}

SDL_Color hexmap_vegetation_color(int vegetation) {
    if (vegetation < 0 || vegetation >= 12) {
        vegetation = 11;  /* Default to none */
    }
    return vegetation_colors[vegetation];
}

SDL_Color hexmap_owner_color(int owner) {
    if (owner < 0 || owner >= 8) {
        return (SDL_Color){100, 100, 100, 255};  /* Neutral gray */
    }
    return owner_colors[owner];
}

void hexmap_grid_to_pixel(hexmap_context_t *ctx, int gx, int gy, int *px, int *py) {
    const zoom_config_t *cfg = &zoom_configs[ctx->zoom];
    
    /* Calculate pixel position with odd-r offset */
    *px = (gx - ctx->view_x) * cfg->xshift;
    
    /* Odd columns are shifted down */
    int odd_offset = ((gx - ctx->view_x) % 2) ? cfg->oddlift : 0;
    *py = (gy - ctx->view_y) * cfg->yshift + odd_offset;
    
    /* Add margin */
    *px += cfg->hex_size;
    *py += cfg->hex_size;
}

void hexmap_pixel_to_grid(hexmap_context_t *ctx, int px, int py, int *gx, int *gy) {
    /* Inverse of grid_to_pixel - approximate for hit testing */
    const zoom_config_t *cfg = &zoom_configs[ctx->zoom];
    
    int adj_x = px - cfg->hex_size;
    int adj_y = py - cfg->hex_size;
    
    *gx = adj_x / cfg->xshift + ctx->view_x;
    
    /* Account for odd-r offset */
    int odd_offset = ((*gx - ctx->view_x) % 2) ? cfg->oddlift : 0;
    *gy = (adj_y - odd_offset) / cfg->yshift + ctx->view_y;
}

int hexmap_render_tile(SDL_Renderer *renderer, hexmap_context_t *ctx,
                       int gx, int gy, int px, int py) {
    if (!ctx || !renderer) return -1;
    
    const zoom_config_t *cfg = &zoom_configs[ctx->zoom];
    int idx = gy * ctx->map_width + gx;
    
    if (idx < 0 || idx >= ctx->map_width * ctx->map_height) {
        return -1;
    }
    
    hex_tile_t *tile = &ctx->tile_cache[idx];
    SDL_Color fill_color;
    SDL_Color outline_color = {50, 50, 50, 255};
    
    /* Determine color based on display mode */
    switch (ctx->display_mode) {
        case DMODE_TERRAIN:
        default:
            /* Blend elevation and vegetation */
            fill_color = hexmap_elevation_color(tile->elevation);
            if (tile->vegetation != 11) {  /* Not "none" */
                SDL_Color veg = hexmap_vegetation_color(tile->vegetation);
                /* Simple blend: average */
                fill_color.r = (fill_color.r + veg.r) / 2;
                fill_color.g = (fill_color.g + veg.g) / 2;
                fill_color.b = (fill_color.b + veg.b) / 2;
            }
            break;
            
        case DMODE_ELEVATION:
            fill_color = hexmap_elevation_color(tile->elevation);
            break;
            
        case DMODE_VEGETATION:
            fill_color = hexmap_vegetation_color(tile->vegetation);
            break;
            
        case DMODE_OWNERSHIP:
            fill_color = hexmap_owner_color(tile->owner);
            break;
    }
    
    /* Draw the hex */
    hexmap_draw_hex(renderer, px, py, cfg->hex_size - 1, fill_color, outline_color);
    
    /* Draw unit indicator if present */
    if (tile->has_army || tile->has_navy) {
        SDL_Rect unit_rect = {px - 6, py - 6, 12, 12};
        SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255);  /* Blue for units */
        SDL_RenderFillRect(renderer, &unit_rect);
    }
    
    /* Draw city indicator if present */
    if (tile->has_city) {
        SDL_Rect city_rect = {px - 8, py - 8, 16, 16};
        SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);  /* Gold for cities */
        SDL_RenderDrawRect(renderer, &city_rect);
        SDL_RenderFillRect(renderer, &city_rect);
    }
    
    return 0;
}

int hexmap_render_cursor(SDL_Renderer *renderer, hexmap_context_t *ctx) {
    if (!ctx || !renderer) return -1;
    
    int px, py;
    hexmap_grid_to_pixel(ctx, ctx->cursor_x, ctx->cursor_y, &px, &py);
    
    const zoom_config_t *cfg = &zoom_configs[ctx->zoom];
    
    /* Draw highlighted hex around cursor */
    hexmap_draw_hex(renderer, px, py, cfg->hex_size, cursor_color, cursor_outline);
    
    /* Draw crosshair */
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderDrawLine(renderer, px - 10, py, px + 10, py);
    SDL_RenderDrawLine(renderer, px, py - 10, px, py + 10);
    
    return 0;
}

int hexmap_render(SDL_Renderer *renderer, hexmap_context_t *ctx) {
    if (!ctx || !renderer) return -1;
    
    /* Clear background */
    SDL_SetRenderDrawColor(renderer, 20, 40, 20, 255);  /* Dark green */
    SDL_RenderClear(renderer);
    
    const zoom_config_t *cfg = &zoom_configs[ctx->zoom];
    
    /* Calculate visible range */
    int screen_w, screen_h;
    SDL_GetRendererOutputSize(renderer, &screen_w, &screen_h);
    
    int cols_visible = screen_w / cfg->xshift + 2;
    int rows_visible = screen_h / cfg->yshift + 2;
    
    int start_x = ctx->view_x;
    int start_y = ctx->view_y;
    int end_x = start_x + cols_visible;
    int end_y = start_y + rows_visible;
    
    /* Clamp to map bounds */
    if (end_x > ctx->map_width) end_x = ctx->map_width;
    if (end_y > ctx->map_height) end_y = ctx->map_height;
    
    /* Render visible tiles */
    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int px, py;
            hexmap_grid_to_pixel(ctx, x, y, &px, &py);
            hexmap_render_tile(renderer, ctx, x, y, px, py);
        }
    }
    
    /* Render cursor on top */
    hexmap_render_cursor(renderer, ctx);
    
    return 0;
}

void hexmap_set_view(hexmap_context_t *ctx, int x, int y) {
    if (!ctx) return;
    ctx->view_x = x;
    ctx->view_y = y;
}

void hexmap_move_view(hexmap_context_t *ctx, int dx, int dy) {
    if (!ctx) return;
    ctx->view_x += dx;
    ctx->view_y += dy;
    
    /* Clamp to valid range */
    if (ctx->view_x < 0) ctx->view_x = 0;
    if (ctx->view_y < 0) ctx->view_y = 0;
    if (ctx->view_x >= ctx->map_width) ctx->view_x = ctx->map_width - 1;
    if (ctx->view_y >= ctx->map_height) ctx->view_y = ctx->map_height - 1;
}

void hexmap_center_on(hexmap_context_t *ctx, int x, int y) {
    if (!ctx) return;
    
    /* Estimate center (simplified) */
    ctx->view_x = x - 5;
    ctx->view_y = y - 5;
    
    if (ctx->view_x < 0) ctx->view_x = 0;
    if (ctx->view_y < 0) ctx->view_y = 0;
}

void hexmap_set_zoom(hexmap_context_t *ctx, int zoom) {
    if (!ctx) return;
    if (zoom < ZOOM_MIN) zoom = ZOOM_MIN;
    if (zoom > ZOOM_MAX) zoom = ZOOM_MAX;
    ctx->zoom = zoom;
}

void hexmap_set_cursor(hexmap_context_t *ctx, int x, int y) {
    if (!ctx) return;
    ctx->cursor_x = x;
    ctx->cursor_y = y;
}

void hexmap_move_cursor(hexmap_context_t *ctx, int dx, int dy) {
    if (!ctx) return;
    ctx->cursor_x += dx;
    ctx->cursor_y += dy;
    
    /* Wrap/clamp */
    if (ctx->cursor_x < 0) ctx->cursor_x = ctx->map_width - 1;
    if (ctx->cursor_x >= ctx->map_width) ctx->cursor_x = 0;
    if (ctx->cursor_y < 0) ctx->cursor_y = ctx->map_height - 1;
    if (ctx->cursor_y >= ctx->map_height) ctx->cursor_y = 0;
}

void hexmap_set_display_mode(hexmap_context_t *ctx, int mode) {
    if (!ctx) return;
    if (mode >= 0 && mode < DMODE_NUMBER) {
        ctx->display_mode = mode;
    }
}

const char *hexmap_mode_name(int mode) {
    static const char *names[] = {
        "Terrain",
        "Elevation",
        "Vegetation",
        "Ownership"
    };
    
    if (mode >= 0 && mode < DMODE_NUMBER) {
        return names[mode];
    }
    return "Unknown";
}
