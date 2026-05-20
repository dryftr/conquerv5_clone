# SPRITE_LAYOUT.md - Conquer V5 Sprite System Reference

## Directory Structure

RAY'S IDEAS:
---------------------------------------------------------------------------
## WORLD MAP TILE CHANGES ##
*THESE CHANGES DO NOT APPLY TO ncurses MODE*

1. MAP SPRITE CHANGES:
peak.png *CHANGED* - uses elevation (mtn_base_#.png) + vegetation (peak.png) combined
volcano.png *CHANGED* - uses an elevation (mtn_base_#.png) + vegetation (volcano_#.png) combined
desert.png *CHANGED* - uses elevation (sand_#.png) + vegetation (cacti_#.png) combined
hill.png *CHANGED* - uses elevation (hill_#.png) + vegetation (hill_veg#.png) combined
barren.png *CHANGED* - is NOW an elevation tile, NOT vegetation

2. POSSIBLE CHANGES:
None

3. ANIMATED TILES
water tile
volcano tile(s)

4. SOME TILES HAVE MULTIPLES TO ALLOW FOR SOME SLIGHT VARIATIONS AND REDUCE THE "TILING" EFFECT OVER MAP REGIONS.
Need to account for numbered tiles, like desert1.png, desert2.png, etc. is it possible to allow for as many as a modder desires, or should there be a cap (like 8).

## Peak Offset / Z-Order Issue (noted 2026-05-15)

Peak.png draws with a slight upward offset to fake height. Problem: the offset pixels bleed into the hex above. If that hex has a building/city, draw order (top-to-bottom rows) means the peak draws AFTER the building, so the peak renders on top of the building above it. That's wrong.

**Options when we implement:**
1. Tighten hex clip mask — contains peak, loses height illusion
2. Multi-pass rendering — elevation bases → overlays → peak tops (buildings always under peaks, which looks correct)
3. Design peak sprites with transparent edges at the offset boundary

Option 2 is cleanest long-term but changes the map renderer. Plan for this.

**Decision (2026-05-15):** Approach A — semi-transparent peak tips handle overlap naturally. Multi-pass rendering confirmed for the architecture.

## Render Pass Order (Confirmed)

All hexes render in layers across the entire map, not per-hex:

| Pass | Content | Notes |
|------|---------|-------|
| 1 | Elevation bases | water, valley, clear, hill, mountain, mtn_base |
| 2 | Vegetation overlays | forest, desert, jungle, etc. (50% alpha) | NOTE: Since vegetation has an transparent background, may not need 50% alpha effect
| 3 | Buildings/designations | city, farm, mine, town, etc. |
| 4 | Peak tops (offset) | Semi-transparent tips for height illusion |
| 5 | Units | Player/AI armies, navy |
| 6+ | Effects (future) | Weather, fire, spell animations |

Peak overlap handled by art: upper pixels of peak sprites are semi-transparent. If a peak tip overlaps a city above, it reads as depth (mountain behind town), not a rendering error.

---

UNITS ARE LIKELY GOING TO BE REPRESENTED BY A SIMPLE ICON FOR "COMMON" UNITS, WITH A COLORING OR SLIGHT GRAPHIC DISTINCTION BETWEEN RACES
Example: INFANTRY USE SWORD & SHIELD ICON (SHIELD DESIGN VARIES BY RACE) OR A BOW ICON FOR ARCHERS (WITH BOW DESIGNS VARIED BY RACES)

---------------------------------------------------------------------------

```
sprites/
├── terrain/
│   ├── elevation/          # 6 types
│   │   ├── water.png  (animated strip)          	X
│   │   ├── valley.png (2 versions)
│   │   ├── clear.png  (make green?)             	X
│   │   ├── hill.png (2 versions)                   X
│   │   ├── mountain.png (versions 3)		        X
│   │   └── mtn_base.png ( base+peak elevation)		X  (used to be peak.png, now mtn_base + peak.png is new COMBO elevation)
|   |   |-- sand.png  (replces desert.png elev)     X
|   |
│   └── vegetation/         # 12 types
│       ├── volcano.png (same as peak.png TWO of these, animated)
│       ├── cacti.png (cacti_v1-4 FEw [4])		    X	
│       ├── tundra.png (few of these 3-4)
│       ├── barren.png (switch with current clear.png?)
│       ├── lt_veg.png (plants/flowers? FEW of these)
│       ├── good.png (????)
│       ├── wood.png (only a few trees? SEVERAL)
│       ├── forest.png (LOTS of these 6-8)
│       ├── jungle.png (LOTS of these 6-8)
│       ├── swamp.png (SEVERAL of these 3-6) 2      X        
│       ├── ice.png (FEW of these 3-4)
|       |-- peak.png (slight up offset to center)	X  (uses slight up offset to fake "massive height" on hex
│       └── none.png (clear png - just terrain shows)
|	
|
├── units/                  # 111 army types (see armyX.h)
│   ├── infantry.png    
│   ├── archers.png
│   ├── militia.png
│   ├── dragon.png
│   ├── demon.png
│   ├── ... (add as needed)
│   └── balrog.png
├── navy/                   # 4 ship classes × 3 sizes
│   ├── warship.png
│   ├── warship_light.png
│   ├── warship_medium.png
│   ├── warship_heavy.png
│   ├── merchants.png
│   ├── galleys.png
│   └── barges.png
├── buildings/              # 15 major + 13 minor designations
│   ├── farm.png
│   ├── fertile.png
│   ├── fruitful.png
│   ├── metalmine.png
│   ├── jewelmine.png
│   ├── lumberyard.png
│   ├── shrine.png
│   ├── bridge.png
│   ├── canal.png
│   ├── wall.png
│   ├── cache.png
│   ├── stockade.png
│   ├── town.png
│   ├── city.png
│   └── capital.png
└── ui/
    ├── cursor.png
    └── selection.png
```

## Naming Convention

| Source | Prefix | Sprite Filename | Example |
|--------|--------|-----------------|---------|
| Elevation enum | `ELE_` | strip prefix, lowercase | `ELE_WATER` → `water.png` |
| Vegetation enum | `VEG_` | strip prefix, lowercase | `VEG_FOREST` → `forest.png` |
| Army class | `AC_` | strip prefix, lowercase | `AC_ARCHERS` → `archers.png` |
| Major desig | `MAJ_` | strip prefix, lowercase | `MAJ_CITY` → `city.png` |
| Minor desig | `MIN_` | strip prefix, lowercase | `MIN_HARBOR` → `harbor.png` |
| Navy ship | `NSHP_` | strip prefix, lowercase | `NSHP_WARSHIPS` → `warship.png` |
| Unit proper name | none | lowercase | "Dragon" → `dragon.png` |

**Rule:** lowercase, underscores for spaces, no special characters.

## sprites.json (Animation Manifest)

Only needed for animated sprites. Static sprites don't need an entry.

```json
{
  "terrain": {
    "elevation": {
      "water": { "frames": 4, "speed": 250, "size": [32, 32] }
    },
    "vegetation": {
      "forest": { "frames": 4, "speed": 200, "size": [32, 32] },
      "jungle": { "frames": 4, "speed": 180, "size": [32, 32] },
      "volcano": { "frames": 6, "speed": 150, "size": [32, 32] }
    }
  },
  "units": {
    "dragon": { "frames": 8, "speed": 80, "size": [48, 48] },
    "demon": { "frames": 6, "speed": 100, "size": [32, 32] }
  }
}
```

### Animation Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `frames` | int | 1 | Number of frames in the sprite strip |
| `speed` | int | 100 | Milliseconds per frame (lower = faster) |
| `size` | [w, h] or int | [32, 32] | Frame dimensions in pixels |

### How Animation Works

- The PNG is a **horizontal sprite strip**: frames side by side
- `infantry.png` at 32×32 with 6 frames = 192×32 total image
- Frame 0 is always the "idle" frame
- Playback loops automatically based on `speed`
- If no `sprites.json` entry exists, the sprite is static (1 frame)

## Sprite Sizes

| Zoom Level | Hex Size | Sprite Scales To |
|-----------|----------|-------------------|
| 0 (far)   | 16px     | 16×16 |
| 1 (medium)| 24px     | 24×24 |
| 2 (close) | 32px     | 32×32 (base) |
| 3 (closest)| 48px    | 48×48 |

**Base art size: 32×32** for most sprites, 48×48 for large units (dragons, balrogs). SDL2 handles scaling automatically at other zoom levels.

## Fallback Behavior

If a PNG is missing:
- A colored rectangle appears (color derived from sprite name hash)
- White 1px border for visibility
- Game never crashes

If a `sprites.json` entry references a sprite that doesn't exist:
- Entry is ignored gracefully

## File Format

- **Format:** PNG only (for now)
- **SVG:** Shelved as "maybe later" — would need nanosvg dependency
- **Transparency:** PNG alpha channel supported
- **Sprite strips:** Horizontal layout, frame 0 = idle

## Adding a New Sprite

1. Drop `your_sprite.png` in the correct subfolder (e.g., `sprites/units/`)
2. If animated, add an entry to `sprites.json`
3. If the game is running, press **R** to hot-reload
4. Done. No code changes needed.

## Currently Loaded Sprites

### Terrain - Elevation (6/6) ✅
| Sprite | File | Status | Animated |
|--------|------|--------|----------|
| water | water.png | ✅ placeholder | Yes (4 frames, 250ms) |
| valley | valley.png | ✅ placeholder | No |
| clear | clear.png | ✅ placeholder | No |
| hill | hill.png | ✅ placeholder | No |
| mountain | mountain.png | ✅ placeholder | No |
| peak | peak.png | ✅ placeholder | No |

Sprite design notes by Ray:

Elevated Terrain tiles can have the "illusion" of height by adding a "layer" to the botton edge of the hexagon, and this can be reversed to add "depth" for tiles like valleys by adding something similar to the top edge. I am going to test examples of this in the hill, mountain, peak, and valley tile sprites. Might look a little "off" with some ground units after offset, but that's fine. Not looking for perfection here, that's one of the "obsessions" that always trips me up with artwork, but not this time. Clearer of mind this time around.

I can do a simple animation for water that does not have to take up the hexagon tiles from edge-to-edge, the waves will almost touch them, but this way I to not have to worry about tiling because it's not that important, the animation will make sense once I've drawn a few examples. I would like it to use at least four frames, that should be enough. Pixel art animations were a bit choppy anyway, that was "the way" then.

Smooth SVG to a "pixelated" PNG will be accomplished by using the PixelOver art program, a conversion tool meant to make artwork that is "realistic" more like old-school pixel art. I feel that style is absolutely appropriate for a game that was originally designed in the late eighties and early ninties when that absolutely WAS the current art style. Modders can change it around and make new style sets, I look forward to it.
 
To use PixelOver, I have to take the SVG art and scale it up by several factors as part of the process (say from 64x64 to 512x512). This is part of how PixelOver operates. When scaled down from larger sizes a bitmap tends to get more "pixelated" and "blurry" and this is how PO works, but makes the illusion more organized and "intentional" looking. I was never really good at pixel art to begin with - it is definitely a niche style and can produce AMAZING results when done by a gifted pixel artist. That I am not. My skills tend to learn more towards realistic or stylized.



### Terrain - Vegetation (12/12) ✅
| Sprite | File | Status | Animated |
|--------|------|--------|----------|
| volcano | volcano.png | ✅ placeholder | Yes (6 frames, 150ms) |
| desert | desert.png | ✅ placeholder | No |
| tundra | tundra.png | ✅ placeholder | No |
| barren | barren.png | ✅ placeholder | No |
| lt_veg | lt_veg.png | ✅ placeholder | No |
| good | good.png | ✅ placeholder | No |
| wood | wood.png | ✅ placeholder | No |
| forest | forest.png | ✅ placeholder | Yes (4 frames, 200ms) |
| jungle | jungle.png | ✅ placeholder | Yes (4 frames, 180ms) |
| swamp | swamp.png | ✅ placeholder | No |
| ice | ice.png | ✅ placeholder | No |
| none | none.png | ✅ placeholder | No |

### Buildings (5/28)
| Sprite | File | Status | Animated |
|--------|------|--------|----------|
| farm | farm.png | ✅ placeholder | No |
| city | city.png | ✅ placeholder | No |
| capital | capital.png | ✅ placeholder | No |
| mine | mine.png | ✅ placeholder | No |
| town | town.png | ✅ placeholder | No |

### Units (0/111)
All using fallback rectangles. Add sprites as you create them.

### Navy (0/4+)
All using fallback rectangles.

### UI (2/2) ✅
| Sprite | File | Status | Animated |
|--------|------|--------|----------|
| cursor | cursor.png | ✅ placeholder | No |
| selection | selection.png | ✅ placeholder | No |

---

*Last updated: 2026-04-28 by Emmi*

