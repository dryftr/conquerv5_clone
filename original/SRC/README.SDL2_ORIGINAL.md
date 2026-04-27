# SDL2 Display Layer (Original Conquer V5)

This directory contains the SDL2-based graphics backend for the modernized Original Conquer V5 (`original/`).

## Purpose
- Replace the original curses-based rendering in `displayG.c` with an SDL2/SDL2_ttf-based renderer.
- Preserve the original game logic (sector, unit, and world data) while modernizing the presentation layer.
- Provide a path forward for Ray’s design notes, textures, fonts, and UI assets.

## Architecture
- **`sdl2/SDL2_display.h`** – Interface and context structure.
- **`sdl2/SDL2_display.c`** – SDL2 backend (init, shutdown, event loop, basic rendering stubs).
- **`sdl2/main_sdl2_original.c`** – Simple test runner (green map, yellow sectors, blue units).
- **`sdl2/Makefile`** – Build system for SDL2 + original code.
- **`SRC/README.SDL2_ORIGINAL.md`** – This file.

## Migration Path
1. **Initialize SDL2:** Use `SDL2_display_init()` to create a window/renderer.
2. **Swap Rendering:** As Ray’s design notes and assets arrive, replace simple colored rectangle stubs in `SDL2_display.c` with texture-based rendering.
3. **Input & Audio:** Add SDL2 input (keyboard/mouse/gamepad) and audio (if desired) in subsequent phases.

## Integration
- `sdl2/SDL2_display.o` is compiled alongside `displayG.c` in `Makefile`.
- A minimal `main_sdl2_original.c` can be built to test the SDL2 backend.
- When ready, `displayG.c` can be recompiled to use SDL2 directly instead of curses.

## Credits
- **Modernization Lead:** Emmi (AI assistant)
- **Original Conquer V5:** Ed Barlow, Adam Bryant, Juan Manuel Méndez Rey (GPL-3.0+)
- **SDL2 Layer:** Built for Ray’s Conquer V5 Modernization project (Phase 4).
- **Design Notes:** See `SPRITE_TYPES.md` and Ray’s personal notes (to be merged into this repo).

## Roadmap (see `SPRITE_TYPES.md`)
- [x] SDL2 Display Backend (Phase 4a)
- [ ] Input Layer (keyboard/mouse/gamepad)
- [ ] Audio (optional, if desired)
- [ ] Networking Revisited (Phase 5)

---
*Built with ❤️ and ☕ in Conquer V5 Modernization Phase 4a–4b.*
