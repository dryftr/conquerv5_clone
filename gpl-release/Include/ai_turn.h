// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * ai_turn.h - AI Turn Orchestrator for Conquer V5
 * Sprint 1 Task 1.5: Turn Pipeline Integration
 *
 * This module orchestrates the full AI turn pipeline:
 *   personality_load → fog_update → evaluate_board → select_strategy
 *   → execute_strategy → diplomacy_evaluate → report_generate
 *
 * It replaces the direct action_expand calls in cpu_update() with
 * a complete turn sequence that considers personality, fog of war,
 * diplomatic state, and strategic priorities.
 *
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */

#ifndef AI_TURN_H
#define AI_TURN_H

/* Note: ai_turn.c includes dataA.h before this header.
 * dataA.h provides all game data structures, macros, and globals.
 * Only include supplemental headers not already in dataA.h's chain. */

/* ============================================================
 * Turn Result Codes
 * ============================================================ */

typedef enum {
  TURN_OK = 0,            /* Turn completed normally */
  TURN_NO_TERRITORY,      /* Nation has no territory to act from */
  TURN_NO_TROOPS,         /* Nation has no armies to command */
  TURN_FOG_NO_TARGETS,    /* No visible targets through fog */
  TURN_PERSONALITY_FAIL,  /* Personality load failed */
  TURN_STRATEGY_FAIL,     /* Strategy selection failed */
  TURN_ERROR              /* General error */
} turn_result_t;

/* ============================================================
 * Strategy Types
 *
 * These represent the high-level strategic posture the AI adopts
 * for a given turn. Personality weights influence which strategy
 * is selected, and the strategy in turn influences which actions
 * are prioritized.
 * ============================================================ */

typedef enum {
  STRAT_EXPAND,       /* Claim new territory, expand borders */
  STRAT_CONSOLIDATE,  /* Build up existing territory, improve economy */
  STRAT_ATTACK,        /* Offensive: target enemy positions */
  STRAT_DEFEND,       /* Defensive: reinforce borders, garrison */
  STRAT_PATROL,       /* Roving: patrol borders, respond to threats */
  STRAT_ECONOMY,      /* Economy focus: maximize production */
  NUM_STRATEGIES       /* Count of valid strategies */
} strategy_type_t;

/* ============================================================
 * Turn Phase Flags
 *
 * Bit flags indicating which phases to execute.
 * Allows partial turns (e.g., economy-only turn after war).
 * ============================================================ */

#define PHASE_FOG          0x01  /* Update fog of war */
#define PHASE_DIPLOMACY    0x02  /* Evaluate diplomatic state */
#define PHASE_STRATEGY     0x04  /* Select strategy */
#define PHASE_EXPAND       0x08  /* Territory claiming */
#define PHASE_MILITARY     0x10  /* Army movement */
#define PHASE_BUILD        0x20  /* Construction */
#define PHASE_ROVE          0x40  /* Rove remaining armies */
#define PHASE_REPORT       0x80  /* Generate turn report */
#define PHASE_ALL          0xFF  /* Execute all phases */

/* ============================================================
 * Turn Context Structure
 *
 * This accumulates state across all phases of the turn,
 * allowing later phases to reference earlier results.
 * ============================================================ */

typedef struct s_turn_context {
  /* Nation context */
  int nation_id;              /* Our nation id (country) */
  int turn_number;            /* Current game turn */

  /* Strategy for this turn */
  strategy_type_t strategy;   /* Selected strategy */
  unsigned int phase_flags;   /* Which phases to execute */

  /* Per-phase results */
  int sectors_claimed;        /* Sectors claimed this turn */
  int armies_moved;           /* Armies moved this turn */
  int buildings_started;      /* Buildings started this turn */
  int roving_armies;          /* Armies sent roving */

  /* Diplomatic assessment */
  int threat_level;           /* 0-100 perceived danger */
  int opportunity_level;      /* 0-100 expansion potential */

  /* Personality weights (cached from PERSONALITY_STRUCT) */
  int weight_military;
  int weight_economy;
  int weight_defense;
  int weight_expansion;

  /* Turn report data */
  char report_summary[256];   /* Brief turn summary for fupdate */
} TURN_CONTEXT, *TURN_CONTEXT_PTR;

/* ============================================================
 * Strategy Selection Weights
 *
 * Each personality type has different weights for each strategy.
 * Higher weight = higher probability of selecting that strategy.
 * These are defaults; can be tuned per-nation.
 * ============================================================ */

/* Warlord: always attacking, rarely consolidating */
#define STRAT_WEIGHT_WARLORD    { 20, 10, 40, 10, 10, 10 }
/* Pioneer: expanding and patrolling */
#define STRAT_WEIGHT_PIONEER   { 35, 15, 10, 10, 20, 10 }
/* Merchant: economy first, defend what you have */
#define STRAT_WEIGHT_MERCHANT  { 5, 20, 5,  15, 10, 45 }
/* Strategist: balanced, adapts to situation */
#define STRAT_WEIGHT_STRATEGIST { 15, 20, 15, 20, 15, 15 }
/* Fortress: defend and consolidate */
#define STRAT_WEIGHT_FORTRESS   { 5, 30, 5,  40, 5,  15 }

/* ============================================================
 * Strategy Decision Factors
 *
 * Situational modifiers that adjust strategy weights.
 * These are added to the base weights based on game state.
 * ============================================================ */

/* Threat modifiers: when enemy strength detected near borders */
#define STRAT_MOD_HIGH_THREAT    { -10, 0, 10, 30, -5, -10 }
/* Opportunity modifiers: when unclaimed land is available */
#define STRAT_MOD_HIGH_OPPORT   { 20, -10, -5, -5, 5, 0 }
/* Low troops: can't attack, consolidate instead */
#define STRAT_MOD_LOW_TROOPS     { -5, 20, -20, 10, 10, 5 }
/* Border pressure: enemy units near border */
#define STRAT_MOD_BORDER_PRESS   { -5, 0, 15, 20, 5, -5 }

/* ============================================================
 * Function Prototypes
 * ============================================================ */

/* Main entry: execute a full AI turn for the current nation */
turn_result_t ai_turn_execute(TURN_CONTEXT_PTR ctx, int nation_id);

/* Phase 1: Load personality and initialize turn context */
turn_result_t ai_turn_init(TURN_CONTEXT_PTR ctx, int nation_id);

/* Phase 2: Update fog of war for this nation */
void ai_turn_update_fog(TURN_CONTEXT_PTR ctx);

/* Phase 3: Evaluate board state and diplomatic situation */
void ai_turn_evaluate(TURN_CONTEXT_PTR ctx);

/* Phase 4: Select strategy based on personality + situation */
strategy_type_t ai_turn_select_strategy(TURN_CONTEXT_PTR ctx);

/* Phase 5: Execute the selected strategy */
turn_result_t ai_turn_execute_strategy(TURN_CONTEXT_PTR ctx);

/* Phase 6: Rove remaining unassigned armies */
int ai_turn_rovers(TURN_CONTEXT_PTR ctx);

/* Phase 7: Generate turn report */
void ai_turn_report(TURN_CONTEXT_PTR ctx);

/* Helper: get strategy name string */
const char *ai_strategy_name(strategy_type_t strategy);

/* Helper: calculate threat level for current nation */
int ai_assess_threat(int nation_id);

/* Helper: calculate opportunity level for current nation */
int ai_assess_opportunity(int nation_id);

#endif /* AI_TURN_H */