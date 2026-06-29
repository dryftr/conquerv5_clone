# Phase 4c: SDL2 Hex Map Rendering System

## Overview

This phase implements hardware-accelerated hexagonal tile rendering for Conquer V5, replacing the original curses-based `hexmapG.c` rendering system with modern SDL2 graphics.

## Files Created

### Core Implementation
- `gpl-release/Src/hexmapG_sdl2.h` - Header file with API and data structures
- `gpl-release/Src/hexmapG_sdl2.c` - Implementation of hex map rendering
- `gpl-release/Src/test_hexmap_sdl2.c` - Test runner with interactive controls

### Makefile Updates
- Added `test-hexmap` target to build and run the hex map test
- Added `hexmapG_sdl2.o` to the object list

## Features Implemented

### 1. Hexagonal Grid Rendering
- Pointy-top hexagon drawing with configurable sizes
- Odd-r offset coordinate system (matching original game)
- 4 zoom levels (16px to 48px hex sizes)
- Hardware-accelerated rendering via SDL2

### 2. Display Modes
Four display modes matching the original game:
- **Terrain (1)**: Blended elevation + vegetation colors
- **Elevation (2)**: Elevation-only view (water to peak)
- **Vegetation (3)**: Vegetation-only view
- **Ownership (4)**: Nation ownership colors

### 3. Interactive Controls
| Key | Action |
|-----|--------|
| Arrow / HJKL | Move cursor |
| WASD | Pan view |
| 1-4 | Change display mode |
| +/- | Zoom in/out |
| c | Center view on cursor |
| r | Regenerate test map |
| ESC / Q | Quit |

### 4. Visual Elements
- Elevation colors: Water (cyan), Valley (light green), Clear (gray), Hill (brown), Mountain (dark gray), Peak (slate)
- Vegetation colors: 12 types from Volcano (red/orange) to Ice (ice blue)
- Unit indicators: Blue squares for armies/navies
- City indicators: Gold-bordered squares
- Cursor highlight: Yellow hex with crosshair

### 5. Data Structures

#### hex_tile_t
```c
typedef struct {
    int elevation;      /* 0-5 */
    int vegetation;     /* 0-11 */
    int designation;    /* Major/minor */
    int owner;          /* Nation (-1 = none) */
    int has_army;
    int has_navy;
    int has_city;
    int highlight;
} hex_tile_t;
```

#### hexmap_context_t
```c
typedef struct {
    int map_width, map_height;
    int view_x, view_y;
    int cursor_x, cursor_y;
    int zoom;
    int display_mode;
    /* ... textures and cache ... */
} hexmap_context_t;
```

## Build Instructions

```bash
cd gpl-release/Src
make test-hexmap
./test_hexmap          # Default 1024x768
./test_hexmap -r 800x600   # Custom resolution
```

## API Reference

### Initialization
```c
int hexmap_init(hexmap_context_t *ctx, int width, int height);
void hexmap_shutdown(hexmap_context_t *ctx);
```

### Rendering
```c
int hexmap_render(SDL_Renderer *renderer, hexmap_context_t *ctx);
int hexmap_render_tile(SDL_Renderer *renderer, hexmap_context_t *ctx,
                       int gx, int gy, int px, int py);
```

### View Control
```c
void hexmap_set_view(hexmap_context_t *ctx, int x, int y);
void hexmap_move_view(hexmap_context_t *ctx, int dx, int dy);
void hexmap_center_on(hexmap_context_t *ctx, int x, int y);
void hexmap_set_zoom(hexmap_context_t *ctx, int zoom);
```

### Cursor
```c
void hexmap_set_cursor(hexmap_context_t *ctx, int x, int y);
void hexmap_move_cursor(hexmap_context_t *ctx, int dx, int dy);
```

### Display Mode
```c
void hexmap_set_display_mode(hexmap_context_t *ctx, int mode);
const char *hexmap_mode_name(int mode);
```

## Color Reference

### Elevation Colors
| ID | Name | Color (SDL) |
|----|------|-------------|
| 0 | Water | (0, 100, 200, 255) |
| 1 | Valley | (144, 238, 144, 255) |
| 2 | Clear | (128, 128, 128, 255) |
| 3 | Hill | (139, 69, 19, 255) |
| 4 | Mountain | (105, 105, 105, 255) |
| 5 | Peak | (112, 128, 144, 255) |

### Vegetation Colors
See `SPRITE_TYPES.md` for full list of 12 vegetation types.

### Owner Colors (8 Nations)
| ID | Color |
|----|-------|
| 0 | Red |
| 1 | Blue |
| 2 | Green |
| 3 | Yellow |
| 4 | Magenta |
| 5 | Cyan |
| 6 | Orange |
| 7 | Purple |

## Next Steps (Phase 4d)

1. **Sprite Integration**: Replace colored rectangles with actual sprite textures
2. **Text Rendering**: Add TTF font support for sector info
3. **UI Overlay**: Implement button rendering and click handling
4. **Animation**: Smooth camera movement and unit animations
5. **Optimization**: Implement dirty rectangle rendering

## Testing

The test program generates a random 40x30 sector map with:
- Random elevation/vegetation distribution
- Random nation ownership
- Scattered units and cities
- Interactive cursor and view controls

## Compatibility

- Matches original `hexmapG.c` coordinate system
- Compatible with existing game data structures
- Ready for integration with `displayG_sdl2.c`

---

*Phase 4c Complete - Hexagonal Tile Rendering System*
*Part of Conquer V5 Modernization - SDL2 Graphics Phase*
