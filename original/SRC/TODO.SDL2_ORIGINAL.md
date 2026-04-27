# TODO.SDL2_ORIGINAL.md

This file tracks the SDL2 graphics modernization for the `original/` Conquer V5 build.

## Phase 4: Graphics Modernization (SDL2)
### Milestone 4a: SDL2 Display Backend
- [x] `sdl2/SDL2_display.h` – Interface and context structure
- [x] `sdl2/SDL2_display.c` – SDL2 init/shutdown/event loop + rendering stubs
- [x] `sdl2/main_sdl2_original.c` – Simple test runner
- [x] `sdl2/Makefile` – Compile `sdl2/SDL2_display.o` alongside `displayG.c`
- [x] `SRC/README.SDL2_ORIGINAL.md` – Migration path and credits
- [x] `SRC/SPRITE_TYPES.md` – Milestones tracker (SDL2 display, input, audio, networking)

### Milestone 4b: Input Layer
- [ ] SDL2 keyboard/mouse input handlers
- [ ] Basic gamepad/joystick support (SDL2 joystick)
- [ ] Integrate with `keybindG.c` and `sectorG.c`

### Milestone 4c: Audio (optional)
- [ ] SDL2 audio initialization
- [ ] Simple synthesized/external SFX integration

## Phase 5: Networking Revisited
- [ ] Integrate Phase 2–3 sockets into the new SDL2 display/input pipeline.
- [ ] Multiplayer turn protocol over UDP/TCP (existing from Phase 3).

## Design Notes (Ray’s Ideas)
- [ ] Merge `notes.md` from Windows into `memory/YYYY-MM-DD.md`.
- [ ] Review and categorize graphics, UI, and level design ideas.
- [ ] Plan texture/asset pipeline (PNG, sprites, fonts).

---
*Built with ❤️ and ☕ in Conquer V5 Modernization Phase 4a–4b.*
