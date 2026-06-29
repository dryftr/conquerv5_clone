# Sprint 2 — Tactical AI & Game Integration

**Status:** PLANNING
**Started:** —
**Depends on:** Sprint 0 ✅, Sprint 1 ✅

---

## Overview

Sprint 2 completes the core NPC AI decision layer by extracting monster behaviors into dedicated modules, adding tactical combat/economic decision-making, and wiring everything into the main game build. After Sprint 2, every NPC turn flows through a full personality → fog → evaluate → strategy → execute → report pipeline inside the running game.

---

## Sprint 0 & 1 Recap (Completed)

### Sprint 0: Honest AI — Warlord Proof of Concept ✅
| Task | Description | Tests |
|------|-------------|-------|
| 0.1 | Personality data structs + loader | 30/30 |
| 0.2 | Fog of war module | 50/50 |
| 0.3 | Decision engine | 26/26 |
| 0.4 | Hook into cpu_update() | wired |
| 0.5 | Turn report generation | 44/44 |

**Total: 150 tests, all passing.**

Files: `Include/ai/personality.h`, `Include/ai/fog_of_war.h`, `Include/ai/decision.h`, `Include/ai/ai_report.h`, `Include/ai/ai_standalone_types.h`, `Include/ai/diplomacy_ai.h`, `Src/ai/personality.c`, `Src/ai/fog_of_war.c`, `Src/ai/decision.c`, `Src/ai/ai_report.c`, `Src/ai/diplomacy_ai.c`, `Src/ai/ai_integration.c`, `Auxil/personalities/*.json`

### Sprint 1: Full Personality Suite + Action Execution ✅
| Task | Description | Tests |
|------|-------------|-------|
| 1.1 | Personality JSON files (5 types) | validated |
| 1.2 | Personality registry | 67/67 |
| 1.3 | AI diplomacy module | 30/30 |
| 1.4 | Real Action Execution — Expand | 55/55 |
| 1.5 | AI Turn Orchestrator | 40/40 |

**Total: 192 tests, all passing.**

Files: `Include/action_expand.h`, `Include/ai_turn.h`, `Src/action_expand.c`, `Src/ai_turn.c`, `Src/test_action_expand.c`, `Src/test_ai_turn.c`, `Src/test_mock_globals.c`, `Src/Makefile.test`

**Cumulative: 342 tests, all passing. Standalone test builds only — not yet in game build.**

---

## Sprint 2 Tasks

### Task 2.1: Monster AI Extraction (2-3 days)

Extract monster behaviors from `npcA.c` into dedicated modules.

**Current state:** All monster logic (lizard, savage, pirate, nomad) lives in `move_for_ntn()` as a giant switch statement.

**Target:**
- `Include/ai/ai_monsters.h` — common monster types and interface
- `Src/ai/ai_lizards.c` — horde behavior (swarm, spread, consume)
- `Src/ai/ai_savages.c` — raid behavior (attack weak targets, retreat from strong)
- `Src/ai/ai_pirates.c` — naval harassment (coastal raids, block ports)
- `Src/ai/ai_nomads.c` — migration behavior (follow resources, avoid conflict)
- `Src/npcA.c` reduced to dispatcher: `move_for_ntn()` → personality AI or monster module
- Each monster type gets its own test suite
- `n_ismonster()` / `n_islizard()` / `n_issavage()` / `n_ispirate()` / `n_isnomad()` checks preserved

**Key design:** Monsters don't use personality system. They use fixed behavior patterns with randomization. Keep it simple.

**Files:**
- New: `Include/ai/ai_monsters.h`, `Src/ai/ai_lizards.c`, `Src/ai/ai_savages.c`, `Src/ai/ai_pirates.c`, `Src/ai/ai_nomads.c`
- Modified: `Src/npcA.c` (strip monster code, add dispatcher calls)

---

### Task 2.2: Tactical Module (3-4 days)

Combat engagement, retreat, and garrison placement.

**Current state:** `ai_move_military()` in `action_expand.c` moves armies toward targets but has no real combat logic. No retreat. No garrison placement.

**Target:**
- `Include/ai/ai_tactical.h` — tactical decision interface
- `Src/ai/ai_tactical.c` — implementation
- Combat engagement: should we attack? Compare attacker vs defender strength, personality thresholds
- Retreat logic: when to pull back (low strength, outnumbered, strategic withdrawal)
- Garrison placement: border sectors get garrisons proportional to threat
- Reinforcement: move reserves to threatened borders
- Fog-of-war enforcement: can only engage visible enemies

**Key structs:**
```c
typedef struct {
    int x, y;
    int priority;
    tactical_target_type_t type;  /* ATTACK, DEFEND, REINFORCE, SCOUT */
    int estimated_strength;
    int confidence;  /* 0-100 based on fog freshness */
} TACTICAL_TARGET;

typedef struct {
    int attack_count;
    int defend_count;
    int retreat_count;
    int scout_count;
    int garrisons_placed;
} TACTICAL_RESULT;
```

**Personality weighting:**
- Warlord: attack at 60% strength ratio, rarely retreat
- Fortress: attack at 120% strength ratio, retreat early
- Pioneer: attack at 80%, retreat if outnumbered
- Merchant: attack at 90%, retreat quickly to protect economy
- Static: attack at 100%, minimal offensive action

**Test suite:** `Src/ai/test_ai_tactical.c`

**Files:**
- New: `Include/ai/ai_tactical.h`, `Src/ai/ai_tactical.c`, `Src/ai/test_ai_tactical.c`
- Modified: `Src/ai_turn.c` (wire tactical module into execute phase)

---

### Task 2.3: Economic Module (2-3 days)

Build prioritization, resource management, construction decisions.

**Current state:** `ai_best_designation()` in `action_expand.c` picks sector types but there's no construction queue or resource balancing.

**Target:**
- `Include/ai/ai_economic.h` — economic decision interface
- `Src/ai/ai_economic.c` — implementation
- Build prioritization by personality (Warlord → armies, Merchant → towns, Fortress → stockades)
- Resource balance: food vs minerals vs gold, adjust spending based on reserves
- Construction queue: what to build next, where, weighted by personality
- Upgrade decisions: when to improve existing sectors vs claim new ones

**Key structs:**
```c
typedef struct {
    int food_production;
    int mineral_income;
    int treasury;
    int expected_expenses;
    int build_queue_size;
} AI_ECONOMY;

typedef struct {
    int farms_started;
    int mines_started;
    int towns_started;
    int stockades_started;
    int upgrades_started;
} ECONOMIC_RESULT;
```

**Personality weighting:**
- Warlord: 50% military buildings, 30% economy, 20% defense
- Fortress: 40% defense, 35% economy, 25% military
- Pioneer: 40% expansion, 30% economy, 30% military
- Merchant: 50% economy, 30% military, 20% defense
- Static: 50% economy, 30% defense, 20% military

**Test suite:** `Src/ai/test_ai_economic.c`

**Files:**
- New: `Include/ai/ai_economic.h`, `Src/ai/ai_economic.c`, `Src/ai/test_ai_economic.c`
- Modified: `Src/ai_turn.c` (wire economic module into execute phase)

---

### Task 2.4: Game Build Integration (1-2 days)

Wire Sprint 0-2 modules into the main Conquer game build.

**Current state:** Sprint 0-1 AI modules (`Src/ai/`) compile standalone with `ai_standalone_types.h` but are NOT in the game Makefile. Sprint 1 modules (`action_expand.c`, `ai_turn.c`) compile in the game build but reference the standalone AI modules.

**Target:**
- Add all `Src/ai/*.c` files to the game `Makefile` SRCS list
- `#ifdef MEMORYH` guards verified for dual compilation (standalone test vs game build)
- Personality JSONs loaded at game init from `Auxil/personalities/`
- Full pipeline runs in-game: personality → fog → evaluate → strategy → expand → tactical → economic → report
- Game compiles and runs with AI modules active
- `cpu_update()` dispatches personality nations through orchestrator, monsters through dedicated modules

**Integration checklist:**
- [ ] `Src/ai/personality.c` in Makefile
- [ ] `Src/ai/fog_of_war.c` in Makefile
- [ ] `Src/ai/decision.c` in Makefile
- [ ] `Src/ai/diplomacy_ai.c` in Makefile
- [ ] `Src/ai/ai_report.c` in Makefile
- [ ] `Src/ai/ai_integration.c` in Makefile
- [ ] `Src/ai/ai_tactical.c` in Makefile (Task 2.2)
- [ ] `Src/ai/ai_economic.c` in Makefile (Task 2.3)
- [ ] `Src/action_expand.c` in Makefile (already done)
- [ ] `Src/ai_turn.c` in Makefile (already done)
- [ ] Personality JSON loading at game start
- [ ] `AIDIR` Makefile variable for personality files
- [ ] Smoke test: start game, AI nations take turns

**Files:**
- Modified: `gpl-release/Src/Makefile`, `gpl-release/Src/npcA.c`
- No new files

---

### Task 2.5: AI-Only Simulation & Balance Testing (2-3 days)

Run AI-only games, observe behavior, tune weights.

**Current state:** No way to run an AI-only game. Need a simulation mode.

**Target:**
- `--ai-only` command-line flag: all nations are AI, no human input needed
- Auto-advance turns with configurable delay
- Log output: per-nation decisions, territory changes, combat results
- Balance tuning based on observed behavior
- At least 3 full AI-only games to completion (or 100 turns each)
- Personality variance verification: different personalities produce meaningfully different outcomes

**Test methodology:**
1. Run Warlord vs Merchant vs Fortress (3 AI nations)
2. Run 5-Way (all personalities, 5 AI nations)
3. Monster-only game (lizards, savages, pirates, nomads)
4. Verify: no crashes, no infinite loops, no stalemates from passive behavior
5. Verify: territory grows, combat occurs, economy develops
6. Verify: Warlord expands aggressively, Fortress hunkers down, Merchant builds economy

**Deliverable:** Balance adjustment report with recommended weight changes.

**Files:**
- New: `Src/ai/test_simulation.c` (simulation runner)
- Modified: `Src/mainG.c` (add `--ai-only` flag), `Src/ai_turn.c` (weight tuning)

---

## Estimated Timeline

| Task | Days | Status |
|------|------|--------|
| 2.1 Monster AI Extraction | 2-3 | Planning |
| 2.2 Tactical Module | 3-4 | Planning |
| 2.3 Economic Module | 2-3 | Planning |
| 2.4 Game Build Integration | 1-2 | Planning |
| 2.5 Simulation & Balance | 2-3 | Planning |
| **Sprint 2 Total** | **10-15** | **Planning** |

## Order of Execution

Recommended order: **2.4 → 2.1 → 2.2 → 2.3 → 2.5**

Rationale: Integration (2.4) first means we catch build issues early and can test each new module in-game as we build it. Monster extraction (2.1) is a clean refactor with well-defined boundaries. Tactical (2.2) and Economic (2.3) build on each other. Simulation (2.5) validates everything together.

---

## Open Design Decisions

1. **Monster behavior fidelity** — How faithful to the original monster logic vs. modernizing? Original is simple random movement. Modernizing could add pack behavior, territory claims, etc.
2. **Construction queue depth** — How many build orders can a nation queue? Original had 1 at a time. Personality-based limits (Warlord queues more military)?
3. **Retreat threshold tuning** — The ratios in Task 2.2 are starting points. Playtesting will determine actual values.
4. **AI-only simulation format** — Text log? JSON replay? Visual replay?
5. **Personality JSON location at runtime** — `AUXDIR/personalities/` alongside other game data? Embedded defaults with JSON overrides?

---

## Dependencies

- **Sprint 0 ✅** — Personality system, fog of war, decision engine
- **Sprint 1 ✅** — Full personality suite, diplomacy, action execution, orchestrator
- **Phase 5 ✅** — Graphics/sprites functional for visual testing
- **Phase 6 spec** — `gpl-release/specs/PHASE6_NPC_AI.md` (architecture reference)

## Related Documents

- `specs/PHASE6_NPC_AI.md` — Full architecture specification
- `EMMI_PLAN.md` — Master modernization plan
- `EMMI_MODERNIZATION_NOTES.md` — Working notes
- Sprint 0 completion notes: `memory/2026-06-27-conquer-sprint0.md`
- Sprint 1 completion notes: `memory/2026-06-27-conquer-sprint1.md`, `memory/2026-06-29-conquer-task14.md`

---

*This document is a living specification. Update as decisions are made and implementation progresses.*