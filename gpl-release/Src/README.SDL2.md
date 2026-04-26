# SDL2 Display Layer (gpl-release/)

This directory contains the SDL2-based graphics backend for the modernized Conquer V5.

## Purpose
- Replace the original curses-based rendering in `displayG.c` with an SDL2/SDL2_ttf-based renderer.
- Preserve the original game logic (sector, unit, and world data) while modernizing the presentation layer.
- Provide a path forward for Ray’s design notes, textures, fonts, and UI assets.

## Architecture
- **`sdl2_display.h`** – Interface and context structure for SDL2 rendering.
- **`sdl2_display.c`** – Implementation of the SDL2 backend (init, shutdown, event loop, basic rendering stubs).
- **`displayG.c`** – Original curses-based renderer (kept for backward compatibility; can be swapped out later).

## Migration Path
1. **Initialize SDL2:** Use `sdl2_init()` to create a window/renderer, then replace curses calls in `displayG.c` with SDL2 equivalents.
2. **Swap Rendering:** As Ray’s design notes and assets arrive, replace simple colored rectangle stubs in `sdl2_display.c` with texture-based rendering.
3. **Input & Audio:** Add SDL2 input (keyboard/mouse/gamepad) and audio (if desired) in subsequent phases.

## Integration
- `sdl2_display.c` is compiled alongside `displayG.c` in `Makefile` (see build section).
- A minimal `main()` can call `sdl2_init()`, render a map, then loop on `sdl2_present()`.
- When ready, `displayG.c` can be recompiled to use SDL2 directly instead of curses.

## Credits
- **Modernization Lead:** Emmi (AI assistant)
- **Original Conquer V5:** Ed Barlow, Adam Bryant, Juan Manuel Méndez Rey (GPL-3.0+)
- **SDL2 Layer:** Built for Ray’s Conquer V5 Modernization project (Phase 4).
- **Design Notes:** See `TODO.SDL2.md` and Ray’s personal notes (to be merged into this repo).

## License
- **SDL2 Layer:** Personal/educational use only. Based on the original Conquer V5 code (GPL-3.0+).
- **Original Code:** `gpl-release/` remains under GPL-3.0+. The modernization commits are personal experiments.

## Roadmap (see `TODO.SDL2.md`)
- [ ] SDL2 Display Backend (Phase 4a)
- [ ] Input Layer (keyboard/mouse/gamepad)
- [ ] Audio (optional, if desired)
- [ ] Networking Revisited (Phase 5)

---
*Built with ❤️ and ☕ in Conquer V5 Modernization Phase 4.*