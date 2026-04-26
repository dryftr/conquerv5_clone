/* Simple SDL2 test runner for Conquer V5 Phase 4
 * 
 * This is a minimal test to verify SDL2 initialization and rendering.
 * It will be replaced by the full game loop once displayG.c is integrated.
 *
 * Build: gcc -o main_sdl2 main_sdl2.c -lSDL2 -lSDL2_ttf
 * Run: ./main_sdl2
 */

#include "sdl2_display.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    sdl2_context_t ctx = {0};
    int quit = 0;
    int width = 800;
    int height = 600;
    const char *title = "Conquer V5 (SDL2) - Phase 4 Test";

    printf("=== Conquer V5 SDL2 Test Runner ===\n");
    printf("Initializing SDL2 at %dx%d...\n", width, height);

    /* Initialize SDL2 */
    if (sdl2_init(&ctx, width, height, title) != 0) {
        fprintf(stderr, "Failed to initialize SDL2\n");
        return 1;
    }

    printf("SDL2 initialized successfully!\n");
    printf("Press ESC or close the window to quit.\n");

    /* Main loop */
    while (!quit) {
        /* Render map */
        sdl2_render_map(&ctx, 0, 0);
        
        /* Render some test sectors */
        sdl2_render_sector(&ctx, 100, 100, 1);
        sdl2_render_sector(&ctx, 250, 150, 2);
        sdl2_render_sector(&ctx, 400, 200, 3);
        
        /* Render some test units */
        sdl2_render_unit(&ctx, 120, 120, 1);
        sdl2_render_unit(&ctx, 270, 170, 2);
        
        /* Render test text */
        SDL_Color white = {255, 255, 255, 255};
        sdl2_render_text(&ctx, 10, 10, "Conquer V5 - SDL2 Test", white);
        
        /* Present */
        sdl2_present(&ctx);
        
        /* Handle events */
        sdl2_handle_events(&ctx, &quit);
        
        /* Simple delay to prevent CPU spinning */
        SDL_Delay(16); /* ~60 FPS */
    }

    printf("Shutting down SDL2...\n");
    sdl2_shutdown(&ctx);
    printf("SDL2 shutdown complete. Goodbye!\n");

    return 0;
}