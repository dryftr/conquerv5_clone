# Phase 5c: Map Polish

## Status: IN PROGRESS

**Started:** 2026-05-02 (cron continuation from Phase 5b completion)
**Depends on:** Phase 5b (Sprite Integration) ✅ COMPLETE
**Blockers:** Real sprite artwork (Ray creating in PixelOver)

## Completed (from Phase 5b)
- ✅ Sprite loader system with hot-reload
- ✅ Layered hex compositing (terrain → vegetation → buildings → units → UI)
- ✅ 25 placeholder sprites (.ppm format)
- ✅ Zoom level offsets for proper sprite positioning
- ✅ Fallback colored rectangles for missing sprites

## Phase 5c Tasks (IN PROGRESS)

### 1. Smooth Zoom Transitions [IMPLEMENTED - 2026-05-03]
**Technical approach:**
- Instead of instant zoom switching, interpolate between zoom configs
- Lerp hex size, spacing over 250ms with ease-out cubic easing
- Track zoom animation state in hexmap context
- Render at fractional zoom levels during transition

**Implementation:** Added `zoom_transition_t` struct to hexmap_sprites_context_t with
- `target_zoom`, `start_zoom`, `progress`, `start_time`, `is_zooming`
- `hexmap_sprites_set_zoom_smooth()` - initiates smooth transition
- `hexmap_sprites_update_zoom_transition()` - updates interpolation
- `hexmap_sprites_get_current_zoom_*()` - gets interpolated values

**Key bindings:** +/- triggers smooth zoom

### 2. Minimap/Overview Display [IMPLEMENTED - 2026-05-03]
**Requirements:**
- Render entire world map at tiny scale (1px per hex)
- Show current view rectangle as yellow overlay
- Toggle with 'M' key

**Position:** Bottom-right corner, 150x100px default

**Implementation:** Added `minimap_t` struct with
- Cached texture rendering 1px per hex
- Owner-aware coloring (mixes elevation + ownership)
- Yellow view rectangle showing current viewport
- `hexmap_sprites_minimap_toggle()`, `_render()`, `_update()`

### 3. Coordinate Overlay [IMPLEMENTED - 2026-05-03]
**Requirements:**
- Toggle with 'C' key
- Show axial coordinates (x,y) on hexes
- Pixel-digit rendering (no SDL_ttf dependency)
- Only visible at larger zoom levels (hex_size >= 24)

**Implementation:** Added `coord_overlay_t` struct with
- Custom 3x5 pixel digit drawing
- `hexmap_sprites_coords_toggle()`, `_render()`
- Smart positioning at hex center

## Code Tasks Ready to Implement

These don't depend on artwork:
1. Zoom transition animation
2. Minimap rendering system
3. Coordinate text overlay
4. Additional key bindings

## Art Tasks (Ray)

Per SPRITE_LAYOUT.md:
- 6 elevation sprites (water needs animation frames)
- 12 vegetation sprites (volcano, forest, jungle animated)
- 111 unit sprites
- 4+ navy sprites
- 28 building sprites
- UI polish

PixelOver workflow: SVG → scale up → pixelate → export PNG

## Next Action

Continue with zoom transition implementation or minimap system.
Ready to proceed when Emmi session resumes.

## Files to Modify
- `Src/hexmapG_sprites.c/h` - Add transition and minimap
- `Src/hexmapG_sdl2.c/h` - Coordinate rendering helpers
- `Src/mainG.c` - Key bindings for toggle features

## Git Status
Branch: master (clean)
Last commit: 8ccf398 Phase 5b: Sprite Integration into Game Rendering
