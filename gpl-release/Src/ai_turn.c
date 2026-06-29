// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * ai_turn.c - AI Turn Orchestrator for Conquer V5
 * Sprint 1 Task 1.5: Turn Pipeline Integration
 *
 * This module orchestrates the full AI turn sequence:
 *   init → fog_update → evaluate → strategy_select
 *   → execute → rove → report
 *
 * Each phase can be individually enabled/disabled via phase_flags,
 * allowing partial turns (e.g., economy-only after war declaration).
 *
 * Strategy selection is personality-weighted with situational modifiers.
 * The execute phase delegates to action_expand for the actual game actions.
 *
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */

#include "dataA.h"
#include "armyX.h"
#include "moveX.h"
#include "activeX.h"
#include "desigX.h"
#include "statusX.h"
#include "action_expand.h"
#include "ai_turn.h"
#include "ai/ai_tactical.h"
#include "ai/ai_economic.h"

/* ============================================================
 * Internal: Strategy Weight Tables
 * ============================================================ */

/* Map ACT_* types to personality array indices.
 * The 5 standard NPC active types map directly:
 *   ACT_STATIC=1, ACT_ENFORCE=2, ACT_OVERT=3, ACT_MOBILE=4, ACT_KILLER=5
 * These correspond to the indices in strat_weights[]. */
typedef enum {
  PTYPE_STATIC = 0,    /* ACT_STATIC=1 */
  PTYPE_FORTRESS,      /* ACT_ENFORCE=2 */
  PTYPE_PIONEER,       /* ACT_OVERT=3 */
  PTYPE_MERCHANT,      /* ACT_MOBILE=4 */
  PTYPE_WARLORD,       /* ACT_KILLER=5 */
  PTYPE_COUNT
} personality_type_idx_t;

#define NUM_PERSONALITY_TYPES PTYPE_COUNT

/* Base weights per personality type, indexed by personality_type_idx_t */
static const int strat_weights[NUM_PERSONALITY_TYPES][NUM_STRATEGIES] = {
  /* STRAT_EXPAND, CONSOLIDATE, ATTACK, DEFEND, PATROL, ECONOMY */
  { 10, 40,  0, 10,  5, 35 },  /* Static */
  {  5, 30,  5, 40,  5, 15 },  /* Fortress (Enforce) */
  { 35, 15, 10, 10, 20, 10 },  /* Pioneer (Overt) */
  {  5, 20,  5, 15, 10, 45 },  /* Merchant (Mobile) */
  { 20, 10, 40, 10, 10, 10 },  /* Warlord (Killer) */
};

/* Situational modifiers */
static const int mod_high_threat[NUM_STRATEGIES] =
  { -10, 0, 10, 30, -5, -10 };
static const int mod_high_opportunity[NUM_STRATEGIES] =
  { 20, -10, -5, -5, 5, 0 };
static const int mod_low_troops[NUM_STRATEGIES] =
  { -5, 20, -20, 10, 10, 5 };
static const int mod_border_pressure[NUM_STRATEGIES] =
  { -5, 0, 15, 20, 5, -5 };

/* ============================================================
 * Internal: Personality type mapping
 * n_aggression() returns 0 (passive) or 1 (aggressive),
 * but we need to map the full active type to a personality slot.
 * ============================================================ */

static personality_type_idx_t
active_to_personality(int active)
{
  switch (active) {
  case ACT_STATIC:   return PTYPE_STATIC;
  case ACT_ENFORCE:  return PTYPE_FORTRESS;
  case ACT_OVERT:    return PTYPE_PIONEER;
  case ACT_MOBILE:   return PTYPE_MERCHANT;
  case ACT_KILLER:   return PTYPE_WARLORD;
  default:           return PTYPE_MERCHANT; /* fallback */
  }
}

/* ============================================================
 * Turn strategy name strings (turn-level, distinct from
 * decision.h AI_STRATEGY_* names)
 * ============================================================ */

static const char *turn_strategy_names[NUM_STRATEGIES] = {
  "Expand",
  "Consolidate",
  "Attack",
  "Defend",
  "Patrol",
  "Economy"
};

const char *
ai_turn_strategy_name(strategy_type_t strategy)
{
  if (strategy >= 0 && strategy < NUM_STRATEGIES)
    return turn_strategy_names[strategy];
  return "Unknown";
}

/* ============================================================
 * Threat Assessment
 *
 * Counts enemy military strength adjacent to our borders.
 * Returns 0-100 scale (0 = no threat, 100 = surrounded).
 * ============================================================ */

int
ai_assess_threat(int nation_id)
{
  long enemy_str = 0;
  long our_str = 0;
  int x, y;

  /* Scan our borders for enemy armies */
  for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
    for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
      if (!XY_ONMAP(x, y)) continue;

      /* Count our total strength */
      if (sct[x][y].owner == nation_id) {
        our_str += ai_friendly_strength(x, y, nation_id, 0);
      }

      /* Check adjacent sectors for enemy strength */
      if (sct[x][y].owner != nation_id && sct[x][y].owner != UNOWNED) {
        /* Adjacent enemy sector — check if we border it */
        if (ai_has_adjacent_owned(x, y, nation_id)) {
          enemy_str += ai_enemy_strength(x, y, nation_id);
        }
      }
    }
  }

  /* Scale to 0-100: threat = enemy strength relative to ours */
  if (our_str == 0) return (enemy_str > 0) ? 100 : 0;

  long ratio = (enemy_str * 100) / our_str;
  if (ratio > 100) ratio = 100;

  return (int)ratio;
}

/* ============================================================
 * Opportunity Assessment
 *
 * Counts unowned/claimable sectors adjacent to our territory.
 * Returns 0-100 scale (0 = no room, 100 = wide open).
 * ============================================================ */

int
ai_assess_opportunity(int nation_id)
{
  int our_sectors = 0;
  int claimable = 0;
  int x, y;

  for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
    for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
      if (!XY_ONMAP(x, y)) continue;

      if (sct[x][y].owner == nation_id) {
        our_sectors++;
      } else if (sct[x][y].owner == UNOWNED &&
                 ai_has_adjacent_owned(x, y, nation_id)) {
        claimable++;
      }
    }
  }

  if (our_sectors == 0) return 0;

  /* Scale: how much room to grow relative to current size */
  long ratio = (claimable * 100) / (our_sectors + claimable);
  if (ratio > 100) ratio = 100;

  return (int)ratio;
}

/* ============================================================
 * Phase 1: Initialize Turn Context
 * ============================================================ */

turn_result_t
ai_turn_init(TURN_CONTEXT_PTR ctx, int nation_id)
{
  if (ctx == NULL) return TURN_ERROR;

  /* Clear the context */
  memset(ctx, 0, sizeof(TURN_CONTEXT));

  ctx->nation_id = nation_id;
  ctx->turn_number = world.turn;
  ctx->phase_flags = PHASE_ALL;  /* Execute all phases by default */
  ctx->strategy = STRAT_EXPAND;  /* Default strategy */

  /* Load personality weights from active type */
  personality_type_idx_t ptype = active_to_personality(ntn_ptr->active);

  ctx->weight_military = strat_weights[ptype][STRAT_ATTACK] +
                         strat_weights[ptype][STRAT_DEFEND];
  ctx->weight_economy = strat_weights[ptype][STRAT_ECONOMY] +
                        strat_weights[ptype][STRAT_CONSOLIDATE];
  ctx->weight_defense = strat_weights[ptype][STRAT_DEFEND];
  ctx->weight_expansion = strat_weights[ptype][STRAT_EXPAND];

  /* Check if we have any territory */
  if (!ai_has_any_territory(nation_id)) {
    ctx->phase_flags &= ~(PHASE_EXPAND | PHASE_MILITARY | PHASE_BUILD);
    return TURN_NO_TERRITORY;
  }

  return TURN_OK;
}

/* ============================================================
 * Phase 2: Update Fog of War
 *
 * v1: Simple visibility based on owned territory + adjacent.
 * Full fog system from Sprint 0 will be integrated later.
 * ============================================================ */

void
ai_turn_update_fog(TURN_CONTEXT_PTR ctx)
{
  /* v1: Fog is implicit in ai_sector_visible().
   * The full fog_of_war module from Sprint 0 will replace this
   * when integrated. For now, ai_sector_visible() checks
   * ownership + adjacency, which is sufficient for expand actions. */

  /* Mark our borders as visible (already done by ai_sector_visible) */
  ctx->threat_level = ai_assess_threat(ctx->nation_id);
  ctx->opportunity_level = ai_assess_opportunity(ctx->nation_id);
}

/* ============================================================
 * Phase 3: Evaluate Board State
 * ============================================================ */

void
ai_turn_evaluate(TURN_CONTEXT_PTR ctx)
{
  /* Threat and opportunity already assessed in fog_update.
   * Future: diplomatic assessment, resource accounting, etc. */
}

/* ============================================================
 * Phase 4: Select Strategy
 *
 * Uses personality base weights modified by situational factors.
 * The strategy with the highest total weight is selected.
 * ============================================================ */

strategy_type_t
ai_turn_select_strategy(TURN_CONTEXT_PTR ctx)
{
  personality_type_idx_t ptype = active_to_personality(ntn_ptr->active);
  int weights[NUM_STRATEGIES];
  int i;

  /* Start with personality base weights */
  for (i = 0; i < NUM_STRATEGIES; i++) {
    weights[i] = strat_weights[ptype][i];
  }

  /* Apply situational modifiers */

  /* High threat: shift toward defense and attack */
  if (ctx->threat_level > 60) {
    for (i = 0; i < NUM_STRATEGIES; i++)
      weights[i] += mod_high_threat[i];
  }

  /* High opportunity: shift toward expansion */
  if (ctx->opportunity_level > 50) {
    for (i = 0; i < NUM_STRATEGIES; i++)
      weights[i] += mod_high_opportunity[i];
  }

  /* Low troops: can't attack, consolidate instead */
  if (ntn_ptr->army_list == NULL) {
    for (i = 0; i < NUM_STRATEGIES; i++)
      weights[i] += mod_low_troops[i];
  }

  /* Border pressure: enemy units near border */
  if (ctx->threat_level > 30 && ctx->threat_level <= 60) {
    for (i = 0; i < NUM_STRATEGIES; i++)
      weights[i] += mod_border_pressure[i];
  }

  /* Clamp all weights to non-negative */
  for (i = 0; i < NUM_STRATEGIES; i++) {
    if (weights[i] < 0) weights[i] = 0;
  }

  /* Select strategy with highest weight */
  strategy_type_t best = STRAT_EXPAND;
  int best_weight = weights[0];
  for (i = 1; i < NUM_STRATEGIES; i++) {
    if (weights[i] > best_weight) {
      best_weight = weights[i];
      best = (strategy_type_t)i;
    }
  }

  ctx->strategy = best;
  return best;
}

/* ============================================================
 * Phase 5: Execute Strategy
 *
 * Dispatches to action_expand for the actual game actions.
 * Strategy influences which phases of expand get priority.
 * ============================================================ */

turn_result_t
ai_turn_execute_strategy(TURN_CONTEXT_PTR ctx)
{
  EXPAND_STATE expand_state;
  expand_result_t r;

  /* Initialize expand system */
  r = action_expand_init(&expand_state, ctx->nation_id);
  if (r != EXPAND_OK) {
    fprintf(fupdate,
            "  AI: Nation %d expand init failed (result %d)\n",
            ctx->nation_id, r);
    return TURN_STRATEGY_FAIL;
  }

  /* Copy strategy influence into expand weights */
  switch (ctx->strategy) {
  case STRAT_EXPAND:
    expand_state.weight_expansion += 20;
    break;
  case STRAT_CONSOLIDATE:
    expand_state.weight_economy += 20;
    expand_state.weight_military -= 10;
    break;
  case STRAT_ATTACK:
    expand_state.weight_military += 20;
    expand_state.attack_threshold -= 20;  /* More aggressive */
    break;
  case STRAT_DEFEND:
    expand_state.weight_defense += 20;
    expand_state.weight_expansion -= 10;
    break;
  case STRAT_PATROL:
    /* Balanced with slight military edge */
    expand_state.weight_military += 10;
    break;
  case STRAT_ECONOMY:
    expand_state.weight_economy += 30;
    expand_state.weight_military -= 10;
    expand_state.weight_expansion -= 10;
    break;
  default:
    break;
  }

  /* Clamp weights to non-negative */
  if (expand_state.weight_military < 10) expand_state.weight_military = 10;
  if (expand_state.weight_economy < 10) expand_state.weight_economy = 10;
  if (expand_state.weight_defense < 10) expand_state.weight_defense = 10;
  if (expand_state.weight_expansion < 10) expand_state.weight_expansion = 10;
  if (expand_state.attack_threshold < 80) expand_state.attack_threshold = 80;

  /* Execute expand pipeline */
  r = action_expand_execute(&expand_state);

  /* Copy results into turn context */
  ctx->sectors_claimed = expand_state.sectors_claimed;
  ctx->armies_moved = expand_state.armies_moved;
  ctx->buildings_started = expand_state.buildings_started;

  if (r == EXPAND_NO_TARGETS) {
    return TURN_FOG_NO_TARGETS;
  }

  return TURN_OK;
}

/* ============================================================
 * Phase 6: Rove Remaining Armies
 *
 * Armies that weren't assigned by the expand module
 * get sent roving using the legacy rove_army() logic.
 * ============================================================ */

int
ai_turn_rovers(TURN_CONTEXT_PTR ctx)
{
  int roved = 0;
  ARMY_PTR ap;

  for (ap = ntn_ptr->army_list; ap != NULL; ap = ap->next) {
    if (ap->strength < 10) continue;  /* Skip non-combat units */

    /* Set the army context for rove_army() */
    army_ptr = ap;
    rove_army();
    roved++;
  }

  ctx->roving_armies = roved;
  return roved;
}

/* ============================================================
 * Phase 7: Generate Turn Report
 * ============================================================ */

void
ai_turn_report(TURN_CONTEXT_PTR ctx)
{
  snprintf(ctx->report_summary, sizeof(ctx->report_summary),
           "Ntn %d: %s | T%d | S:%d M:%d B:%d R:%d | Threat:%d Opp:%d",
           ctx->nation_id,
           ai_turn_strategy_name(ctx->strategy),
           ctx->turn_number,
           ctx->sectors_claimed,
           ctx->armies_moved,
           ctx->buildings_started,
           ctx->roving_armies,
           ctx->threat_level,
           ctx->opportunity_level);

  fprintf(fupdate, "  AI: %s\n", ctx->report_summary);
}

/* ============================================================
 * Phase 5: Tactical Decisions
 *
 * Evaluates attack/retreat/garrison/reinforce decisions based on
 * personality thresholds and fog-of-war-constrained intel.
 * ============================================================ */

int
ai_turn_tactical(TURN_CONTEXT_PTR ctx)
{
  TACTICAL_STATE tac_state;
  int result;

  if (ctx == NULL) return 0;

  result = ai_tactical_init(&tac_state, ctx->nation_id);
  if (result != 0) {
    fprintf(fupdate,
            "  TAC: Nation %d tactical init failed\n",
            ctx->nation_id);
    return 0;
  }

  /* Override attack/retreat thresholds based on strategy */
  switch (ctx->strategy) {
  case STRAT_ATTACK:
    /* Attack strategy: more aggressive thresholds */
    tac_state.attack_ratio -= 10;
    tac_state.retreat_ratio -= 10;
    break;
  case STRAT_DEFEND:
    /* Defend strategy: more cautious thresholds */
    tac_state.attack_ratio += 20;
    tac_state.retreat_ratio += 15;
    break;
  case STRAT_ECONOMY:
    /* Economy strategy: avoid fights unless clearly winning */
    tac_state.attack_ratio += 30;
    tac_state.retreat_ratio += 20;
    break;
  default:
    /* Other strategies: use personality defaults */
    break;
  }

  fprintf(fupdate,
          "  TAC: Nation %d entering tactical phase "
          "(strategy=%s, attack=%d%%, retreat=%d%%)\n",
          ctx->nation_id,
          ai_turn_strategy_name(ctx->strategy),
          tac_state.attack_ratio,
          tac_state.retreat_ratio);

  result = ai_tactical_execute(&tac_state);

  fprintf(fupdate,
          "  TAC: Nation %d tactical phase complete (%d actions)\n",
          ctx->nation_id, result);

  return result;
}

/* ============================================================
 * Phase 6: Economic Decisions
 *
 * Evaluates economy, prioritizes builds, and starts construction
 * based on personality weights and situational modifiers.
 * ============================================================ */

int
ai_turn_economic(TURN_CONTEXT_PTR ctx)
{
  ECON_STATE econ_state;
  int result;

  if (ctx == NULL) return 0;

  result = ai_economic_init(&econ_state, ctx->nation_id);
  if (result != 0) {
    fprintf(fupdate,
            "  ECON: Nation %d economic init failed\n",
            ctx->nation_id);
    return 0;
  }

  fprintf(fupdate,
          "  ECON: Nation %d entering economic phase "
          "(max_builds=%d, reserve=%.0f%%)\n",
          ctx->nation_id,
          econ_state.max_builds_per_turn,
          econ_state.reserve_pct * 100.0);

  result = ai_economic_execute(&econ_state);

  fprintf(fupdate,
          "  ECON: Nation %d economic phase complete (%d builds)\n",
          ctx->nation_id, result);

  return result;
}

/* ============================================================
 * Main Entry: Execute Full AI Turn
 * ============================================================ */

turn_result_t
ai_turn_execute(TURN_CONTEXT_PTR ctx, int nation_id)
{
  turn_result_t r;

  /* Phase 1: Initialize */
  r = ai_turn_init(ctx, nation_id);
  if (r == TURN_NO_TERRITORY) {
    /* No territory, nothing to do except report */
    fprintf(fupdate,
            "  AI: Nation %d has no territory, skipping turn\n",
            nation_id);
    return r;
  }
  if (r != TURN_OK) {
    fprintf(fupdate,
            "  AI: Nation %d init failed (result %d)\n",
            nation_id, r);
    return r;
  }

  /* Phase 2: Update fog of war */
  if (ctx->phase_flags & PHASE_FOG)
    ai_turn_update_fog(ctx);

  /* Phase 3: Evaluate board state */
  if (ctx->phase_flags & PHASE_DIPLOMACY)
    ai_turn_evaluate(ctx);

  /* Phase 4: Select strategy */
  if (ctx->phase_flags & PHASE_STRATEGY)
    ai_turn_select_strategy(ctx);

  fprintf(fupdate, "  AI: Nation %d strategy: %s (threat=%d, opportunity=%d)\n",
          nation_id, ai_turn_strategy_name(ctx->strategy),
          ctx->threat_level, ctx->opportunity_level);

  /* Phase 5: Tactical decisions (attack, retreat, garrison, reinforce) */
  if (ctx->phase_flags & PHASE_TACTICAL)
    ctx->tactical_actions = ai_turn_tactical(ctx);

  /* Phase 6: Economic decisions (build prioritization, resource allocation) */
  if (ctx->phase_flags & PHASE_ECONOMY)
    ctx->econ_builds = ai_turn_economic(ctx);

  /* Phase 7: Execute strategy */
  if (ctx->phase_flags & (PHASE_EXPAND | PHASE_MILITARY | PHASE_BUILD))
    ai_turn_execute_strategy(ctx);

  /* Phase 8: Rove remaining armies */
  if (ctx->phase_flags & PHASE_ROVE)
    ai_turn_rovers(ctx);

  /* Phase 9: Generate report */
  if (ctx->phase_flags & PHASE_REPORT)
    ai_turn_report(ctx);

  return TURN_OK;
}