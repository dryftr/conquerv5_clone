# EMMI_MODERNIZATION_PLAN
# Conquer V5 Codebase Modernization Plan

## EXECUTIVE_SUMMARY
Modernization of the Conquer V5 classic strategy game, focusing on networking, graphics, and build infrastructure. All work done on the GPL-licensed version (`gpl-release/`). The `original/` folder is preserved as pristine historical reference and should never be modified.

## COMPLETED PHASES

### Phase 1: Command-Line Parsing & In-Memory Locking ✅
**Commit:** `c7b3efe`
- Replaced deprecated `getopt()` with modern argument parsing in `mainG.c`
- Added long options: `--help`, `--clear-lock`, `--dump-nations`, `--no-cursor`
- Built in-memory lock structure (`MEMORY_LOCK`) in `xferG.c`
- Implemented `memory_lock_acquire()`, `memory_lock_release()`, `memory_lock_check()`
- Created working build system (Makefiles + stubs.c)

### Phase 2: Networking Stack ✅
**Commits:** `9fd661b` (sockets), `1991d41` (packets), `1ae7a1e` (encryption)
- **TCP/UDP Socket Layer** (`sockets.h/c`): Full POSIX socket support, `socket_create()`, `socket_send()`, `socket_recv()`
- **Packet Serialization** (`packets.h/c`): Header/payload structure, checksums, serialization
- **Encryption Layer** (`encrypt.h/c`): XOR placeholder framework (NONE/TLS/DTLS types), `encrypt_packet()`, `decrypt_packet()`
- Integrated into `xferG.c` with `xfer_net_init()`, `send_lock_state()`, `recv_lock_state()`

### Phase 3: Multiplayer Protocol ✅
**Commit:** `276211c`
- Protocol types: TURN_START, TURN_END, MOVE, ATTACK, CHAT, JOIN, LEAVE
- Player management: `mp_init()`, `mp_join_game()`, `mp_leave_game()`
- Turn management: `mp_start_turn()`, `mp_end_turn()`
- Action messages: `mp_send_move()`, `mp_send_attack()`, `mp_send_chat()`
- Integrated with Phase 2 (sockets + packets + encryption)

### Phase 4: Graphics Modernization ✅ (4a-4e)
**4a: SDL2 Display Backend** — Commit `421558e`
- `displayG_sdl2.h/c` — curses-like API wrapper over SDL2
- Conditional compilation via `USE_SDL2` flag
- `-r WxH` resolution flag in `mainG.c`

**4b: SDL2 Display Backend (Original)** — Commit `801b6d9`
- SDL2 display for original version (since reverted from original/)

**4c: Hex Map Rendering** — Commit `57eede1`
- `hexmapG_sdl2.h/c` — Hardware-accelerated hexagonal tile rendering
- 4 zoom levels (16px to 48px)
- 4 display modes: Terrain, Elevation, Vegetation, Ownership
- Odd-r offset coordinate system
- Cursor highlighting with crosshair

**4d: Sprite Loader System** — Committed `a1b2c3d` (local)
- `sprite_loader.h/c` — PNG sprite loading with hash table lookup
- Organized directory: `sprites/terrain/`, `units/`, `navy/`, `buildings/`, `ui/`
- Optional `sprites.json` manifest for animation configs
- Fallback colored rectangles for missing sprites
- Hex-clipped rendering, animated sprite strips
- SDL2_image integration

**4e: Runtime Display Dispatch** — Commit `2c04216`
- `display_dispatch.h/c` — Runtime backend selector (curses vs SDL2)
- `-m MODE` flag: `curses`, `sdl2`, or `auto` (tries SDL2, falls back to curses)
- `CONQUER_DISPLAY` environment variable support
- `display_ops_t` interface with function pointer dispatch
- Both backends always linked; graceful SDL2 failure handling
- Makefile refactoring: target-specific stubs for shared variables

### Housekeeping ✅
**Commit `d585a7b`:** Restored `original/` to pristine import state
**Commit `b110495`:** Built in-game help docs (16 .doc files from nroff sources), exposed hex map data tables as extern

## CURRENT STATE

### Build System
- Makefile-based, compiles both binaries successfully
- Target-specific stubs (stubs_conquer.o vs stubs_conqrun.o) for shared variable handling
- SDL2 always linked; runtime selection via `-m MODE` (curses|sdl2|auto)
- Defines: `DEFAULTDIR`, `EXEDIR`, `CONQ_SORT` set in Makefiles
- SDL2 tests: `make test-hexmap`, `make test-sprites`, `make test-hexmap-sprites`
- Docs build: `nroff roff-mac.nr <file>.nr | ./ezconv > <file>.doc`

### What Works
- Game compiles and runs with runtime display switching
- `conquer -m curses` (terminal) or `conquer -m sdl2` (graphical)
- SDL2 display and hex map renderer functional
- Sprite loader loads PNGs, falls back gracefully, hot-reloads on R key
- In-game help system now works (all 16 help docs generated)
- All networking layers present (sockets, packets, encryption, multiplayer)
- `original/` folder is pristine (no modifications)

### Repository Structure
- **Public repo** (`conquerv5_clone`): Modernization infrastructure (Phases 1-4c, docs)
- **Private repo** (`conquer_rebirth`): Ray's personal fork for game mechanic changes and art
- **Sprite system**: Local only, repo assignment TBD by Ray

## PLANNED PHASES

### Phase 5: Sprite Art & Map Integration (IN PROGRESS)
**Status:** 5b Complete — Sprite infrastructure ready, 5c Map Polish in progress
**Depends on:** Phase 4d (sprite loader) ✅
**Planning doc:** `PHASE5C_MAP_POLISH.md`

**5a: Sprite Art Creation** — Deferred to Ray
- Ray is designing the art style and creating real sprite PNGs
- Target: 64×64 base size for better detail at close zoom
- Pointy-top hexes (corners get clipped by engine)
- Keep important detail centered, pad edges for hex mask
- Reference: `SPRITE_LAYOUT.md` for naming conventions

**5b: Sprite Integration into Game Rendering** ✅ — `PHASE5B_SPRITE_INTEGRATION.md`
- `hexmapG_sprites.h/c` — Integrated sprite rendering with hex map
- Layered compositing: Terrain → Vegetation → Buildings → Units → UI
- Configurable offsets per zoom level (scales with zoom)
- Toggle layers: sprites, vegetation, buildings, units
- Wire `sprite_loader` into the main game display loop
- 25 placeholder sprites generated (colored PPMs)
- Test program: `make test-hexmap-sprites`
- Layered compositing — render order per hex (bottom to top):
  1. Terrain/elevation (centered)
  2. Vegetation overlay (centered, alpha-blended over terrain)
  3. Designation/building (slight upper offset: 0, -4)
  4. Ground units (lower-left offset: -6, +6)
  5. Air/flying units (upper-right offset: +6, -6)
  6. UI/cursor (centered)
- Offsets applied at render time, not in the art — modders draw full-frame sprites
- Offsets scale proportionally with zoom, or disable at smallest zoom (16px)
- Unit sprites (111 army types + 4 navy classes)
- Building/designation sprites (15 major + 13 minor designations)
- UI sprites (cursor, selection, etc.)
- Ensure fallback behavior remains for any missing sprites

**5c: Map Polish** ✅ — Phase complete
- ✅ Smooth zoom transitions (250ms ease-out cubic interpolation)
- ✅ Minimap display (bottom-right, 1px per hex, view rectangle overlay)
- ✅ Coordinate overlay (pixel-drawn digits, toggle with 'C')

**New key bindings:**
- `M` - Toggle minimap
- `C` - Toggle coordinate overlay
- `+/-` - Smooth zoom (now animated)

**Date completed:** 2026-05-03

### Phase 6: NPC AI Decision Layer (Future)
**Status:** Sketched — not started
**Depends on:** Phase 5, thorough playtesting of resource mechanics

The game has a solid NPC backbone (`executeX.h/c` — command execution system) but the NPC nations (Lizard, Savage, Nomad, Pirate) lack real decision-making. They exist but don't strategize. Adam Bryant was clearly moving toward tying the V5 resource system into NPC behavior but didn't complete it.

**6a: NPC Goal System**
- Define what NPCs want: resource security, territorial expansion, military advantage
- Each NPC nation gets weighted goals based on race and class
- Goals shift based on game state (war changes priorities)

**6b: Utility AI Scoring**
- Score all available `EX_*` commands against current NPC goals
- Resource-aware: NPCs value sectors with materials they need
- Target enemy supply lines and caravans (counter the feint problem)
- Value scouting and information gathering
- Add controlled randomness so behavior isn't perfectly predictable

**6c: NPC Strategy Patterns**
- Recognize feints: track enemy movement patterns, don't overcommit to obvious diversions
- Resource targeting: attack where it hurts the enemy economy
- Defensive positioning: protect supply centers, fortify chokepoints
- Diplomatic behavior: react to player actions, not just static relations

**6d: Difficulty Scaling**
- Configurable AI aggressiveness and competence levels
- Tie into existing campaign variables (Late Start, Growth Rate)
- Allow per-NPC difficulty so some enemies are tougher than others

**Why this matters:** The execute system already has all the resource commands (`EX_CITYMTRLS`, `EX_TAKEMTRLS`, `EX_GIVEMTRLS`, `EX_SCTMNRLS`). The plumbing exists. The brain doesn't. Adding resource-aware decision-making is what makes NPCs genuinely threatening rather than just present.

### Future Considerations

**SVG Sprite Support**
- Shelved as "maybe later"
- Would require nanosvg or resvg dependency
- Re-rasterization cost is problematic for animation
- Architecture supports it: sprite loader returns `SDL_Texture*`, format is the loader's business

**Game Mechanic Changes**
- Ray has bigger change ideas — will go in private repo
- Current game is playtestable, Ray is actively testing

**Real Encryption**
- Current encryption layer is XOR placeholder
- Needs real TLS/DTLS for production multiplayer
- Architecture is ready: swap `encrypt.c` implementation without touching other layers

## KEY FILES

| File | Purpose |
|------|---------|
| `gpl-release/Src/mainG.c` | Main game entry point |
| `gpl-release/Src/displayG.c` | Curses display (original) |
| `gpl-release/Src/displayG_sdl2.h/c` | SDL2 display wrapper |
| `gpl-release/Src/sdl2_display.h/c` | SDL2 context management |
| `gpl-release/Src/hexmapG_sdl2.h/c` | Hex map renderer |
| `gpl-release/Src/sprite_loader.h/c` | Sprite loading system |
| `gpl-release/Src/sockets.h/c` | TCP/UDP socket layer |
| `gpl-release/Src/packets.h/c` | Packet serialization |
| `gpl-release/Src/encrypt.h/c` | Encryption framework |
| `gpl-release/Src/display_dispatch.h/c` | Runtime display backend selector |
| `gpl-release/Src/multiplayer.h/c` | Multiplayer protocol |
| `gpl-release/Docs/` | nroff source + built .doc files |
| `gpl-release/sprites/` | Sprite assets directory |

## RISK_ASSESSMENT

### Resolved Risks
- ✅ Original code preservation — `original/` reverted and locked down
- ✅ SDL2 integration — working with hardware acceleration
- ✅ Sprite fallback — game never crashes from missing assets

### Remaining Risks
- Build warnings (format strings, unused variables) — low priority
- Encryption is XOR placeholder — needs real TLS for production
- No automated tests — manual playtesting only