/* Simple SDL2 test runner for Original Conquer V5 */

#include "sdl2/SDL2_display.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    sdl2_context_t ctx = {0};
    int quit = 0;
    int width = 800;
    int height = 600;
    const char *title = "Conquer V5 (SDL2 - Original) - Phase 4a Test";

    printf("=== Conquer V5 SDL2 Test Runner (Original) ===\n");
    printf("Initializing SDL2 at %dx%d...\n", width, height);

    if (SDL2_display_init(&ctx, width, height, title) != 0) {
        fprintf(stderr, "Failed to initialize SDL2\n");
        return 1;
    }

    printf("SDL2 initialized successfully!\n");
    printf("Press ESC or close the window to quit.\n");

    /* Main loop */
    while (!quit) {
        SDL2_display_render_map(&ctx, 0, 0);
        SDL2_display_render_sector(&ctx, 100, 100, 1);
        SDL2_display_render_sector(&ctx, 250, 150, 2);
        SDL2_display_render_sector(&ctx, 400, 200, 3);
        SDL2_display_render_unit(&ctx, 120, 120, 1);
        SDL2_display_render_unit(&ctx, 270, 170, 2);
        SDL_Color white = {255, 255, 255, 255};
        SDL2_display_render_text(&ctx, 10, 10, "Conquer V5 - SDL2 Test (Original)", white);
        SDL2_display_present(&ctx);
        SDL2_display_handle_events(&ctx, &quit);
        SDL_Delay(16); /* ~60 FPS */
    }

    printf("Shutting down SDL2...\n");
    SDL2_display_shutdown(&ctx);
    printf("SDL2 shutdown complete. Goodbye!\n");

    return 0;
}