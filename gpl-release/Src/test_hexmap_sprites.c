/*
 * test_hexmap_sprites.c - Interactive Test for Sprite-Integrated Hexmap
 * Phase 5b: Test Program
 *
 * Run: ./test_hexmap_sprites -r 1024x768
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hexmapG_sprites.h"
#include "sdl2_display.h"

#define DEFAULT_WIDTH 1024
#define DEFAULT_HEIGHT 768

static void generate_test_map(hexmap_sprites_context_t *ctx) {
    srand(time(NULL));
    
    for (int y = 0; y < ctx->base.map_height; y++) {
        for (int x = 0; x < ctx->base.map_width; x++) {
            int idx = y * ctx->base.map_width + x;
            hex_tile_t *tile = &ctx->base.tile_cache[idx];
            
            /* Random elevation biased toward clear/hill */
            int r = rand() % 100;
            if (r < 10) tile->elevation = 0;      /* water */
            else if (r < 20) tile->elevation = 1; /* valley */
            else if (r < 60) tile->elevation = 2; /* clear */
            else if (r < 80) tile->elevation = 3; /* hill */
            else if (r < 95) tile->elevation = 4; /* mountain */
            else tile->elevation = 5;             /* peak */
            
            /* Random vegetation */
            tile->vegetation = rand() % 12;
            
            /* Random units/buildings */
            tile->has_army = (rand() % 20 == 0);
            tile->has_navy = (tile->elevation == 0 && rand() % 10 == 0);
            tile->has_city = (rand() % 50 == 0);
            tile->designation = tile->has_city ? 2 : (rand() % 10 == 0 ? rand() % 5 : 0);
            tile->owner = rand() % 8;
            tile->highlight = 0;
        }
    }
}

static void print_help(void) {
    printf("\n=== Hexmap Sprites Test Controls ===\n");
    printf("Arrow keys / HJKL: Move cursor\n");
    printf("WASD: Pan view\n");
    printf("+/-: Zoom in/out (smooth)\n");
    printf("1-4: Display modes (Terrain, Elevation, Vegetation, Ownership)\n");
    printf("F: Toggle fallback mode (sprites on/off)\n");
    printf("V: Toggle vegetation layer\n");
    printf("B: Toggle buildings layer\n");
    printf("U: Toggle units layer\n");
    printf("M: Toggle minimap\n");
    printf("C: Toggle coordinate overlay\n");
    printf("R: Reload sprites (hot-reload)\n");
    printf("Q/ESC: Quit\n");
    printf("====================================\n\n");
}

int main(int argc, char *argv[]) {
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    const char *sprite_path = "./sprites";
    
    /* Parse args */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            if (sscanf(argv[i+1], "%dx%d", &width, &height) != 2) {
                fprintf(stderr, "Invalid resolution format. Use: -r WIDTHxHEIGHT\n");
                return 1;
            }
            i++;
        }
    }
    
    /* Initialize SDL2 */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    
    /* Create window and renderer */
    SDL_Window *window = SDL_CreateWindow(
        "Conquer V5 - Hexmap Sprites Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    /* Initialize hexmap with sprites */
    hexmap_sprites_context_t ctx;
    if (hexmap_sprites_init(&ctx, renderer, 40, 30, sprite_path) != 0) {
        fprintf(stderr, "Failed to initialize hexmap with sprites\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    /* Generate test map */
    generate_test_map(&ctx);
    
    printf("\n=== Hexmap Sprites Test Initialized ===\n");
    printf("Map size: %dx%d sectors\n", ctx.base.map_width, ctx.base.map_height);
    printf("Loaded %d sprites\n", ctx.sprites ? ctx.sprites->sprite_count : 0);
    printf("Resolution: %dx%d\n", width, height);
    print_help();
    
    /* Main loop */
    int running = 1;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                    
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_q:
                        case SDLK_ESCAPE:
                            running = 0;
                            break;
                            
                        /* Cursor movement */
                        case SDLK_UP:
                        case SDLK_k:
                            hexmap_move_cursor(&ctx.base, 0, -1);
                            break;
                        case SDLK_DOWN:
                        case SDLK_j:
                            hexmap_move_cursor(&ctx.base, 0, 1);
                            break;
                        case SDLK_LEFT:
                        case SDLK_h:
                            hexmap_move_cursor(&ctx.base, -1, 0);
                            break;
                        case SDLK_RIGHT:
                        case SDLK_l:
                            hexmap_move_cursor(&ctx.base, 1, 0);
                            break;
                            
                        /* View panning */
                        case SDLK_w:
                            hexmap_move_view(&ctx.base, 0, -2);
                            break;
                        case SDLK_s:
                            hexmap_move_view(&ctx.base, 0, 2);
                            break;
                        case SDLK_a:
                            hexmap_move_view(&ctx.base, -2, 0);
                            break;
                        case SDLK_d:
                            hexmap_move_view(&ctx.base, 2, 0);
                            break;
                            
                        /* Zoom */
                        case SDLK_PLUS:
                        case SDLK_EQUALS:
                        case SDLK_KP_PLUS:
                            hexmap_sprites_set_zoom_smooth(&ctx, ctx.base.zoom + 1);
                            printf("Zoom: %d\n", ctx.base.zoom);
                            break;
                        case SDLK_MINUS:
                        case SDLK_KP_MINUS:
                            hexmap_sprites_set_zoom_smooth(&ctx, ctx.base.zoom - 1);
                            printf("Zoom: %d\n", ctx.base.zoom);
                            break;
                            
                        /* Display modes */
                        case SDLK_1:
                            hexmap_set_display_mode(&ctx.base, DMODE_TERRAIN);
                            printf("Mode: Terrain\n");
                            break;
                        case SDLK_2:
                            hexmap_set_display_mode(&ctx.base, DMODE_ELEVATION);
                            printf("Mode: Elevation\n");
                            break;
                        case SDLK_3:
                            hexmap_set_display_mode(&ctx.base, DMODE_VEGETATION);
                            printf("Mode: Vegetation\n");
                            break;
                        case SDLK_4:
                            hexmap_set_display_mode(&ctx.base, DMODE_OWNERSHIP);
                            printf("Mode: Ownership\n");
                            break;
                            
                        /* Toggle layers */
                        case SDLK_f:
                            ctx.use_sprites = !ctx.use_sprites;
                            printf("Sprites: %s\n", ctx.use_sprites ? "ON" : "OFF (fallback)");
                            break;
                        case SDLK_v:
                            ctx.show_vegetation = !ctx.show_vegetation;
                            printf("Vegetation: %s\n", ctx.show_vegetation ? "ON" : "OFF");
                            break;
                        case SDLK_b:
                            ctx.show_buildings = !ctx.show_buildings;
                            printf("Buildings: %s\n", ctx.show_buildings ? "ON" : "OFF");
                            break;
                        case SDLK_u:
                            ctx.show_units = !ctx.show_units;
                            printf("Units: %s\n", ctx.show_units ? "ON" : "OFF");
                            break;
                            
                        /* Reload sprites */
                        case SDLK_r:
                            printf("Reloading sprites...\n");
                            hexmap_sprites_reload(&ctx);
                            printf("Reloaded %d sprites\n", ctx.sprites ? ctx.sprites->sprite_count : 0);
                            break;
                            
                        /* Toggle minimap */
                        case SDLK_m:
                            hexmap_sprites_minimap_toggle(&ctx);
                            printf("Minimap: %s\n", ctx.minimap.enabled ? "ON" : "OFF");
                            break;
                            
                        /* Toggle coordinate overlay */
                        case SDLK_c:
                            hexmap_sprites_coords_toggle(&ctx);
                            printf("Coordinates: %s\n", ctx.coords.enabled ? "ON" : "OFF");
                            break;
                            
                        case SDLK_SLASH:
                        case SDLK_QUESTION:
                            print_help();
                            break;
                    }
                    break;
            }
        }
        
        /* Render frame */
        hexmap_sprites_render(renderer, &ctx);
        SDL_RenderPresent(renderer);
        
        /* Frame delay for ~60 FPS */
        SDL_Delay(16);
    }
    
    /* Cleanup */
    hexmap_sprites_shutdown(&ctx);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    
    printf("\nTest completed.\n");
    return 0;
}
