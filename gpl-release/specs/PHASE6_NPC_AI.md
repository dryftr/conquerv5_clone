# Phase 6: NPC AI Decision Layer Architecture

**Status:** IN PLANNING — Awaiting Ray's go/no-go decision  
**Last Updated:** 2026-05-25  
**Author:** Emmi (on behalf of Ray's vision)

---

## Executive Summary

Phase 6 introduces a modern, modular AI decision framework for computer-controlled nations in Conquer V5. The existing `npcA.c` contains placeholder logic and unimplemented functions wrapped in `#ifdef NOT_DONE`. This phase will replace the legacy stubs with a decision tree system that can be extended for different nation personalities (aggressive, defensive, expansionist, economic).

---

## Current State Analysis

### Existing Code (`gpl-release/Src/npcA.c`)

**Implemented:**
- `move_for_ntn()` — Main dispatch function with monster types (lizard, savage, pirate, nomad)
- `rove_army()` — Simple roaming behavior for units
- `rover_value()` — Basic sector attractiveness calculation
- Military stats structure (`NPCINFO_STRUCT`) for troop accounting

**Unimplemented (wrapped in `#ifdef NOT_DONE`):**
- `assign_strategy()` — No strategy selection logic
- `sector_duties()` — No sector role assignment
- `buildup()` — No troop/building construction
- `check_rovers()` — Incomplete rover management
- `stabilize()` — No defensive positioning
- `expand()` — No offensive logic
- `npc_distribute()` — No resource distribution
- `npc_construct()` — No construction queue

**Key Structures:**
```c
typedef struct s_npcinfo {
  long total_troops;
  long rov_troops;  // roving/unassigned
  long att_troops;  // attack allocation
  long def_troops;  // defense allocation
  long gar_troops;  // garrison allocation
} NPCINFO_STRUCT;

typedef struct s_target {
  maptype mapx, mapy;
  int value;
  struct s_target *next;
} TARGET_STRUCT;
```

---

## Proposed Architecture

### Design Principles

1. **Modularity** — Each AI subsystem is independent and swappable
2. **Personality-Driven** — Different nations behave differently based on traits
3. **Goal-Oriented** — AI works toward objectives (conquest, defense, economy)
4. **Playable** — AI provides challenge without being frustrating
5. **Extensible** — Easy to add new strategies and behaviors

### Component Hierarchy

```
npc_ai.h/c (main controller)
├── Strategy Module
│   └── ai_strategy.h/c — Personality selection, goal setting
├── Tactical Module  
│   └── ai_tactical.h/c — Unit movement, combat decisions
├── Economic Module
│   └── ai_economic.h/c — Resource management, construction
├── Diplomatic Module (Phase 6b)
│   └── ai_diplomatic.h/c — Alliance/trade logic
└── Monster Types (existing)
    ├── ai_lizards.c — Horde behavior
    ├── ai_savages.c — Raid behavior
    ├── ai_pirates.c — Naval harassment
    └── ai_nomads.c — Migration behavior
```

---

## Phase 6a: Core AI Framework (Foundation)

### 6a.1 Strategy Selection System

**File:** `ai_strategy.h/c`

```c
typedef enum {
    AI_PERSONALITY_AGGRESSIVE,   // Prioritize attack, fast expansion
    AI_PERSONALITY_DEFENSIVE,    // Fortify borders, tech up
    AI_PERSONALITY_ECONOMIC,     // Maximize production, trade
    AI_PERSONALITY_EXPANSIONIST, // Claim territory, steady growth
    AI_PERSONALITY_BALANCED      // Adaptive based on situation
} ai_personality_t;

typedef enum {
    AI_GOAL_EARLY_EXPANSION,     // Turns 1-20: claim land
    AI_GOAL_MIDGAME_CONSOLIDATION, // Turns 21-50: build up
    AI_GOOL_LATE_DOMINATION    // Turn 51+: attack to win
} ai_goal_phase_t;

typedef struct {
    ai_personality_t personality;
    ai_goal_phase_t current_phase;
    int threat_assessment;      // 0-100 perceived danger
    int opportunity_rating;     // 0-100 expansion potential
    int economic_health;        // 0-100 resource situation
    NationId primary_enemy;
    NationId potential_ally;
} ai_strategy_t;

void ai_select_strategy(ai_strategy_t* strategy, Nation* nation);
void ai_update_phase(ai_strategy_t* strategy, int turn_number);
int ai_assess_threat(const Nation* nation);
int ai_assess_opportunity(const Nation* nation);
```

### 6a.2 Tactical Decision Engine

**File:** `ai_tactical.h/c`

```c
typedef struct {
    int x, y;
    int priority;           // 0-100 how important
    enum {
        TGT_ATTACK,         // Enemy to attack
        TGT_DEFEND,         // Sector to defend
        TGT_CAPTURE,        // Empty land to claim
        TGT_REINFORCE,      // Friendly sector needing troops
        TGT_EXPLORE         // Unknown territory
    } type;
} ai_target_t;

// Target evaluation
void ai_evaluate_targets(const ai_strategy_t* strategy, 
                         ai_target_t* targets, 
                         int* target_count);
int ai_sector_value(int x, int y, const ai_strategy_t* strategy);

// Unit movement
void ai_move_armies(const ai_strategy_t* strategy);
void ai_move_navy(const ai_strategy_t* strategy);
bool ai_should_attack(const Army* attacker, const Army* defender);

// Defensive positioning
void ai_position_garrisons(const ai_strategy_t* strategy);
void ai_establish_perimeter(Nation* nation);
```

### 6a.3 Economic Management

**File:** `ai_economic.h/c`

```c
typedef struct {
    int food_production;
    int mineral_income;
    int treasury;
    int expected_expenses;
    int build_queue_size;
} ai_economy_t;

void ai_manage_economy(ai_strategy_t* strategy, ai_economy_t* economy);
void ai_prioritize_construction(const ai_strategy_t* strategy);
void ai_balance_troops_vs_infrastructure(const ai_strategy_t* strategy);
void ai_distribute_resources(Nation* nation);

// Build decisions
enum BuildingType ai_choose_building(int x, int y, const ai_strategy_t* strategy);
int ai_choose_unit_type(const ai_strategy_t* strategy, int resources);
```

---

## Phase 6b: Enhanced Behaviors (Stretch Goals)

### 6b.1 Diplomatic Logic

- Alliance formation based on shared enemies
- Trade resource surpluses
- Treaty negotiation
- Betrayal logic (backstab allies when advantageous)

### 6b.2 Adaptive Learning

- Remember player strategies across games
- Adjust tactics based on failure/success
- Counter-strategy detection and response

### 6b.3 Difficulty Scaling

```c
typedef enum {
    AI_DIFFICULTY_EASY,      // Makes mistakes, slower expansion
    AI_DIFFICULTY_MEDIUM,    // Balanced play
    AI_DIFFICULTY_HARD,      // Optimized decisions
    AI_DIFFICULTY_EXPERT     // Near-perfect play, sees fog
} ai_difficulty_t;
```

---

## Integration Plan

### Step 1: Refactor Existing Code

1. Extract existing `rove_army()` into `ai_tactical.c`
2. Move monster behaviors to separate files
3. Create `npc_ai.c` as main dispatcher

### Step 2: Implement Strategy Module

1. Personality selection based on nation traits
2. Phase transition logic (early → mid → late game)
3. Threat/opportunity assessment

### Step 3: Implement Tactical Module

1. Target evaluation system
2. Movement decision tree
3. Combat engagement rules
4. Garrison placement

### Step 4: Implement Economic Module

1. Resource balancing
2. Build prioritization
3. Unit recruitment logic

### Step 5: Testing & Balancing

1. Unit tests for each decision function
2. Simulation mode (AI-only games)
3. Balance adjustments based on playtesting

---

## Files to Create/Modify

### New Files
- `gpl-release/Include/ai_strategy.h`
- `gpl-release/Include/ai_tactical.h`
- `gpl-release/Include/ai_economic.h`
- `gpl-release/Src/ai_strategy.c`
- `gpl-release/Src/ai_tactical.c`
- `gpl-release/Src/ai_economic.c`
- `gpl-release/Src/ai_lizards.c` (extract from npcA.c)
- `gpl-release/Src/ai_savages.c` (extract from npcA.c)
- `gpl-release/Src/ai_pirates.c` (extract from npcA.c)
- `gpl-release/Src/ai_nomads.c` (extract from npcA.c)

### Modified Files
- `gpl-release/Src/npcA.c` — Refactor to use new modules
- `gpl-release/Src/Makefile` — Add new source files

---

## Testing Strategy

### Unit Tests
```c
// test_ai_strategy.c
void test_aggressive_personality_selects_attack();
void test_defensive_personality_prioritizes_forts();
void test_phase_transition_at_turn_20();
void test_threat_assessment_detects_bordering_enemy();

// test_ai_tactical.c
void test_target_evaluation_prefers_capitals();
void test_army_avoids_stronger_enemies();
void test_garrison_placement_on_borders();
```

### Integration Tests
- Run AI-only simulation games
- Monitor for crashes or infinite loops
- Validate resource consistency

### Balance Testing
- AI vs AI matches with different personalities
- Human vs AI difficulty calibration
- Adjust aggression/economy ratios

---

## Dependencies & Risks

### Dependencies
- ✅ Phase 5 complete (graphics rendering)
- Phase 6a can proceed in parallel with Phase 6b/6c
- Phase 6b+ recommended after playtesting reveals AI behavior issues

### Risks
| Risk | Mitigation |
|------|------------|
| AI too predictable | Multiple personalities + randomization |
| AI too hard/easy | Difficulty scaling settings |
| Performance lag | Cache decisions, update incrementally |
| Code complexity | Modular design, clear separation |

---

## Decision Required from Ray

**Before proceeding, need clarification on:**

1. **Priority:** Is Phase 6 a blocker for multiplayer release, or can it come after?
2. **Scope:** Do you want the full Phase 6a-6c, or just 6a (basic AI) for now?
3. **Personality Types:** Any specific nation behaviors you want modeled?
4. **Difficulty:** Should the AI see through fog of war, or play fair?
5. **Multiplayer:** Should AI nations be playable slots in multiplayer, or strictly single-player?

---

## Timeline Estimate

| Phase | Estimated Time | Status |
|-------|---------------|--------|
| 6a.1 Strategy Module | 2-3 days | Not started |
| 6a.2 Tactical Module | 3-4 days | Not started |
| 6a.3 Economic Module | 2-3 days | Not started |
| 6a.4 Integration & Testing | 2-3 days | Not started |
| **Phase 6a Total** | **9-13 days** | **Planning** |
| Phase 6b Enhanced | +5-7 days | Deferred |
| Phase 6c Difficulty | +2-3 days | Deferred |

---

## Related Documents

- `EMMI_PLAN.md` — Overall modernization plan
- `PHASE5C_MAP_POLISH.md` — Previous phase (complete)
- `gpl-release/Src/npcA.c` — Existing AI code
- `gpl-release/Include/dataA.h` — Nation data structures

---

*This document is a living specification. Update as decisions are made and implementation progresses.*
