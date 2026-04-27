# SPRITE_TYPES.md

This file documents the in-game types that will become your sprites for testing and production.

## Sector Types

### Elevation (6 types)
| ID | Name    | Symbol (curses) | Suggested Color   |
|----|---------|-----------------|-------------------|
| 0  | Water   | `~`             | Cyan (0x00FFFF)   |
| 1  | Valley  | `v`             | Light Green (0x90EE90) |
| 2  | Clear   | `c`             | Gray (0x808080)   |
| 3  | Hill    | `h`             | Brown (0x8B4513)  |
| 4  | Mountain| `m`             | Dark Gray (0x696969) |
| 5  | Peak    | `p`             | Slate Gray (0x708090) |

### Vegetation (12 types)
| ID | Name      | Symbol (curses) | Suggested Color      |
|----|-----------|-----------------|----------------------|
| 0  | Volcano   | `V`             | Red/Orange (0xFF4500)|
| 1  | Desert    | `d`             | Sandy (0xF4A460)     |
| 2  | Tundra    | `t`             | Lavender (0xE6E6FA)  |
| 3  | Barren    | `b`             | Dark Brown (0x5C4033)|
| 4  | Light Veg | `l`             | Pale Green (0x98FB98)|
| 5  | Good      | `g`             | Forest Green (0x228B22) |
| 6  | Wood      | `w`             | Dark Yellow (0xBDB76B)|
| 7  | Forest    | `f`             | Deep Green (0x006400)|
| 8  | Jungle    | `j`             | Lime (0x32CD32)      |
| 9  | Swamp     | `s`             | Teal (0x008080)      |
| 10 | Ice       | `i`             | Ice Blue (0xADD8E6)  |
| 11 | None      | `n`             | Black/Transparent    |

### Minor Designations (12 types)
| ID | Name           | Suggested Icon | Notes                      |
|----|----------------|----------------|----------------------------|
| 0  | Devastated     | `X`            | Destroyed sector           |
| 1  | ForSale        | `F`            | Ready for trade            |
| 2  | Sieged         | `S`            | Under siege                |
| 3  | TradingPost    | `T`            | Minor trade hub            |
| 4  | Roads          | `R`            | Transport link             |
| 5  | Blacksmith     | `B`            | Minor craft center         |
| 6  | University     | `U`            | Minor knowledge hub        |
| 7  | Church         | `C`            | Minor faith center         |
| 8  | Mill           | `M`            | Minor production           |
| 9  | Granary        | `G`            | Minor food storage         |
| 10 | Fortified      | `!`            | Minor defense              |
| 11 | Harbor         | `H`            | Minor water entry          |

### Major Designations (16 types)
| ID | Name        | Suggested Icon | Notes                      |
|----|-------------|----------------|----------------------------|
| 0  | Farm        | `fa`           | Basic food                 |
| 1  | Fertile     | `fi`           | Enhanced food              |
| 2  | Fruitful    | `fu`           | Best food                  |
| 3  | MetalMine   | `mt`           | Basic metals               |
| 4  | JewelMine   | `mj`           | Best metals                |
| 5  | LumberYard  | `lu`           | Basic wood                 |
| 6  | Shrine      | `sh`           | Minor faith                |
| 7  | Bridge      | `br`           | Minor transport            |
| 8  | Canal       | `ca`           | Minor water transport      |
| 9  | Wall        | `wa`           | Minor defense              |
| 10 | Cache       | `ch`           | Minor storage              |
| 11 | Stockade    | `st`           | Minor troop holding        |
| 12 | Town        | `tn`           | Basic city                 |
| 13 | City        | `ci`           | Medium city                |
| 14 | Capital     | `cp`           | Top-tier capital           |

---

## Unit Types (85 total)

### Leaders (36 types)
Leaders have special classes and roles:
- **AC_LEADER**: Ruler (e.g., King, Emperor, Pope)
- **AC_SPELLCASTER**: Magic (e.g., Wizard, Mage, Sorcerer, Magician)
- **AC_MERCS**: Mercenary leaders
- **AC_SCOUT**: Scout type (e.g., Demon, Devil, Dragyn, Wyrm, Shadow)
- **AC_NAZGUL**: Nazgul type
- **AC_SORCERER**: Sorcerer type
- **AC_MAG**: Magician type
- **AC_SPIRIT**: Spirit type
- **AC_ASSASSIN**: Assassin type
- **AC_EFREET**: Efreet type
- **AC_GARGOYLE**: Gargoyle type
- **AC_WRAITH**: Wraith type
- **AC_HERO**: Hero type
- **AC_CENTAUR**: Centaur type
- **AC_LICH**: Lich type
- **AC_GIANT**: Giant type
- **AC_SUPERHERO**: SuperHero type
- **AC_MUMMY**: Mummy type
- **AC_EARTHMENTAL**: Earthmental type
- **AC_MINOTAUR**: Minotaur type
- **AC_DAEMON**: Daemon type
- **AC_BALROG**: Balrog type
- **AC_DRAGON**: Dragon type

### Normal Units (27 types)
- **AC_NORMAL**: Basic (Militia, Goblins, Orcs, Infantry, Sailors, Marines, Assault, Archers, Uruk-Hai, Ninjas, Longbowmen, Phalanx, Bow_Phalanx, Olog-Hai)

### Specialist Units (22 types)
- **AC_SCOUT**: Scouts, mages, maulers, etc.

---

## UI Types (Buttons)

### Standard UI Sizes
| Element       | Size (pixels) | Notes                         |
|---------------|---------------|-------------------------------|
| Button (small)| 64×32         | Basic UI button               |
| Button (med)  | 80×40         | Text-heavy buttons            |
| Button (large)| 120×60        | Dialog boxes, menus           |
| Icon (small)  | 16×16         | Icons for units/sectors       |
| Icon (med)    | 24×24         | Sector/unit previews          |
| Icon (large)  | 48×48         | Map tile (optional)           |
| Checkbox      | 20×16         | Small toggle                  |
| Tooltip       | 200×48        | Context help text             |

### Suggested Button Text
- **Draft**: "Draft", "Recruit", "Create"
- **Combine**: "Combine", "Merge"
- **Disband**: "Disband", "Release"
- **Move**: "Move", "Relocate"
- **Attack**: "Attack", "Engage"
- **Defend**: "Defend", "Guard"
- **Build**: "Build", "Construct"
- **Train**: "Train", "Upgrade"
- **Trade**: "Trade", "Exchange"
- **Research**: "Research", "Discover"
- **Travel**: "Travel", "Move"
- **Info**: "Info", "Details"

---

## Recommended Test Sprite Set

### Minimal Set for Testing
- **Sectors**: 4×4 grid of elevation+vegetation combos (6×12=72 combos → pick 12 representative ones)
- **Units**: 1 leader + 4 normals (e.g., King, Militia, Infantry, Archers, Uruk-Hai)
- **UI**: 6 buttons (Draft, Combine, Disband, Move, Attack, Info)

### Quick-Start Sprite Pack
- **Sectors**: 20 tiles (4 elevation × 5 vegetation, plus 4 cities, 4 minor designations, 2 borders)
- **Units**: 10 units (4 leaders, 6 normals)
- **UI**: 12 buttons (basic menu)

---

## Notes for Ray’s Design Notes

When you create sprites, consider:
- **Elevation + Vegetation** = 1 sector sprite (e.g., `water_desert.png`)
- **City Types** = Major designations + elevation (e.g., `city_hill_volcano.png`)
- **Unit Types** = Faction-specific (e.g., `King_human.png`, `Militia_human.png`)

---

## File Structure (gpl-release/Src/)

```
Src/
  ├─ sdl2_display.h
  ├─ sdl2_display.c
  ├─ main_sdl2.c
  ├─ main_sdl2_original.c (coming soon)
  ├─ SDL2_display_original.c (coming soon)
  ├─ SDL2_display_original.h (coming soon)
  ├─ README.SDL2.md
  ├─ TODO.SDL2.md
  └─ SPRITE_TYPES.md
```

---

*Built with ❤️ and ☕ in Conquer V5 Modernization Phase 4a–4b.*
