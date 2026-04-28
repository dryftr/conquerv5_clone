/*
 * test_hexmap_sdl2.c - Test runner for SDL2 Hex Map Rendering
 * Phase 4c: Hexagonal Tile Rendering System Test
 */

#include "sdl2_display.h"
#include "hexmapG_sdl2.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Test map dimensions */
#define TEST_MAP_WIDTH  40
#define TEST_MAP_HEIGHT 30

/* Generate test map data */
static void generate_test_map(hexmap_context_t *ctx) {
    srand((unsigned int)time(NULL));
    
    for (int y = 0; y < ctx->map_height; y++) {
        for (int x = 0; x < ctx->map_width; x++) {
            int idx = y * ctx->map_width + x;
            hex_tile_t *tile = &ctx->tile_cache[idx];
            
            /* Generate some terrain variation */
            int noise = rand() % 100;
            
            if (noise < 20) {
                tile->elevation = 0;  /* Water */
            } else if (noise < 40) {
                tile->elevation = 1;  /* Valley */
            } else if (noise < 70) {
                tile->elevation = 2;  /* Clear */
            } else if (noise < 85) {
                tile->elevation = 3;  /* Hill */
            } else if (noise < 95) {
                tile->elevation = 4;  /* Mountain */
            } else {
                tile->elevation = 5;  /* Peak */
            }
            
            /* Vegetation */
            tile->vegetation = rand() % 12;
            
            /* Owner (0-7 or -1 for none) */
            if (rand() % 3 == 0) {
                tile->owner = rand() % 8;
            } else {
                tile->owner = -1;
            }
            
            /* Units */
            tile->has_army = (rand() % 10 == 0);
            tile->has_navy = (rand() % 20 == 0);
            tile->has_city = (rand() % 15 == 0);
            tile->highlight = 0;
        }
    }
    
    /* Place a test city and army at cursor */
    int cx = ctx->cursor_x;
    int cy = ctx->cursor_y;
    int idx = cy * ctx->map_width + cx;
    ctx->tile_cache[idx].has_city = 1;
    ctx->tile_cache[idx].has_army = 1;
    ctx->tile_cache[idx].owner = 0;  /* Player 0 */
}

static void print_help(void) {
    printf("\n=== Conquer V5 Hex Map Test Controls ===\n");
    printf("Arrow keys     - Move cursor\n");
    printf("WASD / HJKL    - Pan view\n");
    printf("1-4            - Change display mode\n");
    printf("    1: Terrain\n");
    printf("    2: Elevation\n");
    printf("    3: Vegetation\n");
    printf("    4: Ownership\n");
    printf("+/-            - Zoom in/out\n");
    printf("c              - Center view on cursor\n");
    printf("r              - Regenerate map\n");
    printf("ESC / Q        - Quit\n");
    printf("=========================================\n\n");
}

int main(int argc, char *argv[]) {
    sdl2_context_t sdl_ctx = {0};
    hexmap_context_t hex_ctx = {0};
    int quit = 0;
    
    printf("=== Conquer V5 SDL2 Hex Map Test ===\n");
    printf("Phase 4c: Hexagonal Tile Rendering\n\n");
    
    /* Initialize SDL2 */
    int width = 1024;
    int height = 768;
    
    /* Check for resolution flag */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            int w, h;
            if (sscanf(argv[i + 1], "%dx%d", &w, &h) == 2) {
                width = w;
                height = h;
                printf("Using resolution: %dx%d\n", width, height);
            }
            i++;
        }
    }
    
    if (SDL2_display_init(&sdl_ctx, width, height, 
                          "Conquer V5 - Hex Map Test (Phase 4c)") != 0) {
        fprintf(stderr, "Failed to initialize SDL2\n");
        return 1;
    }
    
    printf("SDL2 initialized: %dx%d\n", width, height);
    
    /* Initialize hex map */
    if (hexmap_init(&hex_ctx, TEST_MAP_WIDTH, TEST_MAP_HEIGHT) != 0) {
        fprintf(stderr, "Failed to initialize hex map\n");
        SDL2_display_shutdown(&sdl_ctx);
        return 1;
    }
    
    printf("Hex map initialized: %dx%d sectors\n", TEST_MAP_WIDTH, TEST_MAP_HEIGHT);
    
    /* Generate test data */
    generate_test_map(&hex_ctx);
    printf("Test map generated\n");
    
    print_help();
    
    /* Main loop */
    SDL_Event e;
    int needs_render = 1;
    
    while (!quit) {
        /* Handle events */
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;
                    
                case SDL_KEYDOWN:
                    needs_render = 1;
                    switch (e.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            quit = 1;
                            break;
                            
                        /* Cursor movement */
                        case SDLK_UP:
                        case SDLK_k:
                            hexmap_move_cursor(&hex_ctx, 0, -1);
                            break;
                        case SDLK_DOWN:
                        case SDLK_j:
                            hexmap_move_cursor(&hex_ctx, 0, 1);
                            break;
                        case SDLK_LEFT:
                        case SDLK_h:
                            hexmap_move_cursor(&hex_ctx, -1, 0);
                            break;
                        case SDLK_RIGHT:
                        case SDLK_l:
                            hexmap_move_cursor(&hex_ctx, 1, 0);
                            break;
                            
                        /* View panning */
                        case SDLK_w:
                            hexmap_move_view(&hex_ctx, 0, -1);
                            break;
                        case SDLK_s:
                            hexmap_move_view(&hex_ctx, 0, 1);
                            break;
                        case SDLK_a:
                            hexmap_move_view(&hex_ctx, -1, 0);
                            break;
                        case SDLK_d:
                            hexmap_move_view(&hex_ctx, 1, 0);
                            break;
                            
                        /* Display modes */
                        case SDLK_1:
                            hexmap_set_display_mode(&hex_ctx, DMODE_TERRAIN);
                            printf("Mode: Terrain\n");
                            break;
                        case SDLK_2:
                            hexmap_set_display_mode(&hex_ctx, DMODE_ELEVATION);
                            printf("Mode: Elevation\n");
                            break;
                        case SDLK_3:
                            hexmap_set_display_mode(&hex_ctx, DMODE_VEGETATION);
                            printf("Mode: Vegetation\n");
                            break;
                        case SDLK_4:
                            hexmap_set_display_mode(&hex_ctx, DMODE_OWNERSHIP);
                            printf("Mode: Ownership\n");
                            break;
                            
                        /* Zoom */
                        case SDLK_EQUALS:
                        case SDLK_PLUS:
                            hexmap_set_zoom(&hex_ctx, hex_ctx.zoom + 1);
                            printf("Zoom: %d\n", hex_ctx.zoom);
                            break;
                        case SDLK_MINUS:
                            hexmap_set_zoom(&hex_ctx, hex_ctx.zoom - 1);
                            printf("Zoom: %d\n", hex_ctx.zoom);
                            break;
                            
                        /* Center view */
                        case SDLK_c:
                            hexmap_center_on(&hex_ctx, hex_ctx.cursor_x, hex_ctx.cursor_y);
                            printf("Centered on cursor: (%d, %d)\n", 
                                   hex_ctx.cursor_x, hex_ctx.cursor_y);
                            break;
                            
                        /* Regenerate map */
                        case SDLK_r:
                            generate_test_map(&hex_ctx);
                            printf("Map regenerated\n");
                            break;
                            
                        default:
                            needs_render = 0;
                            break;
                    }
                    break;
            }
        }
        
        /* Render */
        if (needs_render) {
            hexmap_render(sdl_ctx.renderer, &hex_ctx);
            SDL_RenderPresent(sdl_ctx.renderer);
            needs_render = 0;
        }
        
        /* Frame rate limit (~60 FPS) */
        SDL_Delay(16);
    }
    
    printf("\nShutting down...\n");
    
    /* Cleanup */
    hexmap_shutdown(&hex_ctx);
    SDL2_display_shutdown(&sdl_ctx);
    
    printf("Phase 4c test complete. Goodbye!\n");
    
    return 0;
}
