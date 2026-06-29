# Sprint 2 Task 2.3 — Economic Module + JSON Wiring

## Goal
Build the Economic Module (build priorities, resource balancing, construction queues) AND retroactively wire all AI modules to read from personality JSON instead of hardcoded constants. Add difficulty multiplier support.

## Why Both?
The personality JSONs already have `attack_preference`, `retreat_threshold`, `build_*`, `priority_*` — but the code uses hardcoded `#define` thresholds and `switch` statements. Fixing this now means modders (and us) can tweak balance by editing JSON, no recompile. Difficulty is just a multiplier layer on top.

## Files to Create
- `Include/ai/ai_economic.h` — Economic module header
- `Src/ai/ai_economic.c` — Economic module implementation
- `Src/ai/test_ai_economic.c` — Standalone tests

## Files to Modify
- `Src/ai/ai_tactical.c` — Replace `set_attack_thresholds()` switch with JSON-driven init
- `Src/action_expand.c` — Replace hardcoded `pref_*` switch with JSON-driven init
- `Include/action_expand.h` — Add economy fields to `EXPAND_STATE`
- `Src/ai/ai_integration.c` — Expose registry accessor for other modules
- `Include/ai/personality.h` — Add difficulty multiplier struct
- `Auxil/personalities/*.json` — Add `garrison_threshold`, `reserve_pct`, `max_builds_per_turn`

## Difficulty Design
A `DIFFICULTY_CONFIG` struct with multipliers applied to AI behavior:
- `ai_economy_mult`: 0.5 (easy) to 1.5 (hard)
- `ai_attack_mult`: 0.7 (easy) to 1.3 (hard)
- `ai_reserve_pct_mult`: 0.5 (easy) to 2.0 (hard)
- Applied AFTER personality JSON values, so "Warlord on Easy" is still aggressive but less efficient

## Pipeline Order
Init → Fog → Evaluate → Strategy → **Tactical** → **Economic** → Execute → Rove → Report

## Economic Module Phases
1. **Evaluate economy**: Read treasury, income, expenses, sector designations
2. **Score builds**: For each owned sector, score every possible improvement
3. **Priority queue**: Sort by (personality_weight × build_preference + situational_modifier)
4. **Budget check**: Skip builds that would drop below reserve threshold
5. **Execute**: Build top N improvements (N = personality-based cap)