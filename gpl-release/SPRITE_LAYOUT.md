# SPRITE_LAYOUT.md - Conquer V5 Sprite System Reference

## Directory Structure

```
sprites/
├── terrain/
│   ├── elevation/          # 6 types
│   │   ├── water.png
│   │   ├── valley.png
│   │   ├── clear.png
│   │   ├── hill.png
│   │   ├── mountain.png
│   │   └── peak.png
│   └── vegetation/         # 12 types
│       ├── volcano.png
│       ├── desert.png
│       ├── tundra.png
│       ├── barren.png
│       ├── lt_veg.png
│       ├── good.png
│       ├── wood.png
│       ├── forest.png
│       ├── jungle.png
│       ├── swamp.png
│       ├── ice.png
│       └── none.png
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