// SPDX-License-Identifier: GPL-3.0-or-later
/* decision.c - AI decision engine implementation */
/*
 * Conquer Reborn - Honest AI
 * Sprint 0: Warlord proof of concept
 *
 * The decision engine is the brain of the honest AI:
 * 1. evaluate_board() - reads world state through fog of war
 * 2. select_strategy() - blends personality with situation
 * 3. execute_strategy() - routes to action functions
 *
 * Sprint 0: Only AI_EXPAND is fully wired. Other strategies are stubs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ai/ai_standalone_types.h"
#include "ai/decision.h"
#include "ai/personality.h"
#include "ai/fog_of_war.h"

/* ------------------------------------------------------------------ */
/* Strategy names for debugging                                        */
/* ------------------------------------------------------------------ */

const char *ai_strategy_name[] = {
  "EXPAND",
  "ATTACK",
  "DEFEND",
  "ECONOMY",
  "DIPLOMACY",
  "SCOUT",
  "HYBRID",
  NULL
};

/* ------------------------------------------------------------------ */
/* Board Evaluation                                                    */
/* ------------------------------------------------------------------ */

int
evaluate_board(BOARD_EVAL_PTR eval, FOW_PTR fog,
               PERSONALITY_PTR pers, ntntype nation_id)
{
  if (!eval || !fog) return -1;

  memset(eval, 0, sizeof(BOARD_EVAL_STRUCT));

  /* Sprint 0: Simplified board evaluation.
   *
   * In the full integration (task 0.4), this will read from the
   * actual Conquer world state through fog of war. For Sprint 0,
   * we provide a test harness that populates the board eval directly.
   *
   * The evaluation framework is here - the wiring to real game data
   * happens when we hook into cpu_update().
   */

  /* Count visible and known sectors from fog state */
  eval->visible_sectors = fog->visible_count;
  eval->known_sectors = fog->known_count;

  /* Threat level: base calculation from visible enemy data.
   * In full integration: count enemy units in visible sectors,
   * compare to own military strength.
   */
  if (eval->total_military > 0 && eval->visible_enemy_military > 0) {
    double ratio = (double)eval->visible_enemy_military / (double)eval->total_military;
    eval->threat_level = ratio > 1.0 ? 1.0 : ratio;
  } else {
    eval->threat_level = 0.0;
  }

  /* Expansion opportunity: based on unclaimed adjacent sectors.
   * In full integration: count unowned sectors bordering owned territory.
   */
  if (eval->owned_sectors > 0 && eval->unclaimed_adjacent >= 0) {
    eval->expansion_opportunity = (double)eval->unclaimed_adjacent /
                                   ((double)eval->owned_sectors * 2.0);
    if (eval->expansion_opportunity > 1.0) eval->expansion_opportunity = 1.0;
  } else {
    eval->expansion_opportunity = 0.5; /* unknown = moderate opportunity */
  }

  /* Economic potential: average efficiency of owned sectors.
   * In full integration: average sct->efficiency across owned sectors.
   */
  eval->economic_potential = eval->avg_efficiency / 100.0;

  return 0;
}

/* ------------------------------------------------------------------ */
/* Strategy Selection                                                   */
/* ------------------------------------------------------------------ */

/* Compute situational weights based on board evaluation.
 * Public for testing - the decision engine's adaptation depends on this. */
void
compute_situational_weights(BOARD_EVAL_PTR eval, PRIORITY_PTR situational)
{
  if (!eval || !situational) return;

  memset(situational, 0, sizeof(PRIORITY_STRUCT));

  /* High threat → boost defense and military, reduce expansion */
  if (eval->threat_level > 0.5) {
    situational->defense = 0.35;
    situational->military = 0.30;
    situational->expansion = 0.10;
    situational->economy = 0.10;
    situational->diplomacy = 0.05;
    situational->scouting = 0.05;
    situational->research = 0.05;
  }
  /* Moderate threat → balanced with defense awareness */
  else if (eval->threat_level > 0.2) {
    situational->defense = 0.20;
    situational->military = 0.20;
    situational->expansion = 0.25;
    situational->economy = 0.15;
    situational->diplomacy = 0.05;
    situational->scouting = 0.10;
    situational->research = 0.05;
  }
  /* Low threat → prioritize expansion and economy */
  else {
    situational->military = 0.10;
    situational->defense = 0.05;
    situational->expansion = 0.35;
    situational->economy = 0.25;
    situational->diplomacy = 0.05;
    situational->scouting = 0.10;
    situational->research = 0.10;
  }

  /* Visible enemies → shift more toward military */
  if (eval->visible_enemy_nations > 0) {
    situational->military += 0.05 * eval->visible_enemy_nations;
    situational->expansion -= 0.03 * eval->visible_enemy_nations;
  }

  /* Many unclaimed sectors → boost expansion */
  if (eval->unclaimed_adjacent > 5) {
    situational->expansion += 0.05;
  }

  /* Weak neighbors → boost attack over expansion */
  if (eval->weak_neighbor_sectors > 3) {
    situational->military += 0.08;
    situational->expansion -= 0.05;
  }

  /* High economic potential → boost economy */
  if (eval->economic_potential > 0.6) {
    situational->economy += 0.05;
  }

  /* Unknown borders → boost scouting */
  if (eval->bordering_unknown > 2) {
    situational->scouting += 0.05;
  }

  /* Clamp all weights to [0, 1] */
  if (situational->military < 0) situational->military = 0;
  if (situational->defense < 0) situational->defense = 0;
  if (situational->expansion < 0) situational->expansion = 0;
  if (situational->economy < 0) situational->economy = 0;
  if (situational->diplomacy < 0) situational->diplomacy = 0;
  if (situational->scouting < 0) situational->scouting = 0;
  if (situational->research < 0) situational->research = 0;
}

/* Pick the highest-weighted strategy */
static AIStrategy
pick_strategy(PRIORITY_PTR priorities)
{
  if (!priorities) return AI_HYBRID;

  /* Map priority domains to strategies */
  struct {
    double weight;
    AIStrategy strategy;
  } candidates[] = {
    { priorities->military,  AI_ATTACK },
    { priorities->defense,  AI_DEFEND },
    { priorities->expansion, AI_EXPAND },
    { priorities->economy,  AI_ECONOMY },
    { priorities->diplomacy, AI_DIPLOMACY },
    { priorities->scouting, AI_SCOUT },
  };

  double max_weight = -1.0;
  AIStrategy best = AI_HYBRID;

  for (int i = 0; i < 6; i++) {
    if (candidates[i].weight > max_weight) {
      max_weight = candidates[i].weight;
      best = candidates[i].strategy;
    }
  }

  return best;
}

/* Compute resource allocation from effective priorities */
static void
compute_resource_allocation(DECISION_PTR decision, PRIORITY_PTR priorities)
{
  if (!decision || !priorities) return;

  double total = priorities->military + priorities->expansion +
                 priorities->defense + priorities->economy +
                 priorities->scouting;

  if (total <= 0) total = 1.0; /* avoid division by zero */

  decision->pct_military = priorities->military / total;
  decision->pct_expansion = priorities->expansion / total;
  decision->pct_defense = priorities->defense / total;
  decision->pct_economy = priorities->economy / total;
  decision->pct_scouting = priorities->scouting / total;
}

int
select_strategy(DECISION_PTR decision, PERSONALITY_PTR pers,
                BOARD_EVAL_PTR eval, FOW_PTR fog)
{
  if (!decision || !pers || !eval) return -1;

  memset(decision, 0, sizeof(DECISION_STRUCT));

  /* Step 1: Compute situational weights from board state */
  PRIORITY_STRUCT situational;
  compute_situational_weights(eval, &situational);

  /* Step 2: Blend base personality with situational weights */
  personality_get_effective_weight(pers, &situational,
                                   &decision->effective_priorities);

  /* Step 3: Pick strategy based on highest effective priority */
  decision->strategy = pick_strategy(&decision->effective_priorities);

  /* Step 4: Compute confidence based on how dominant the top priority is */
  double max_w = 0;
  double total_w = 0;
  PRIORITY_PTR ep = &decision->effective_priorities;
  double weights[] = {
    ep->military, ep->defense, ep->expansion,
    ep->economy, ep->diplomacy, ep->scouting
  };
  for (int i = 0; i < 6; i++) {
    total_w += weights[i];
    if (weights[i] > max_w) max_w = weights[i];
  }
  decision->confidence = (total_w > 0) ? (max_w / total_w) : 0.5;

  /* Step 5: Compute resource allocation */
  compute_resource_allocation(decision, &decision->effective_priorities);

  /* Step 6: Generate reasoning string */
  snprintf(decision->reasoning, sizeof(decision->reasoning),
    "Strategy %s (confidence %.0f%%). "
    "Threat level: %.0f%%, Expansion opportunity: %.0f%%. "
    "Priorities: mil=%.2f exp=%.2f def=%.02f eco=%.2f scout=%.2f dip=%.02f",
    ai_strategy_name[decision->strategy],
    decision->confidence * 100.0,
    eval->threat_level * 100.0,
    eval->expansion_opportunity * 100.0,
    ep->military, ep->expansion, ep->defense,
    ep->economy, ep->scouting, ep->diplomacy);

  /* Step 7: Set target coordinates to (0,0) (no target in Sprint 0) */
  decision->primary_target_x = 0;
  decision->primary_target_y = 0;
  decision->secondary_target_x = 0;
  decision->secondary_target_y = 0;

  return 0;
}

/* ------------------------------------------------------------------ */
/* Strategy Execution                                                   */
/* ------------------------------------------------------------------ */

/* Sprint 0: Only AI_EXPAND has real implementation.
 * Other strategies are stubs that report and do nothing.
 */

static int
execute_expand(DECISION_PTR decision, PERSONALITY_PTR pers,
               BOARD_EVAL_PTR eval, FOW_PTR fog, ntntype nation_id)
{
  /* EXPAND strategy: claim unowned sectors, build military units,
   * send scouts to adjacent unknown territory.
   *
   * Sprint 0 implementation:
   * - Report the expansion plan
   * - In full integration (0.4+), this will issue actual game orders
   */
  if (!decision || !pers) return -1;

  /* Expansion plan based on personality and board state */
  fprintf(stderr,
    "[AI] Nation %d EXPAND: alloc mil=%.0f%% exp=%.0f%% def=%.0f%% eco=%.0f%% scout=%.0f%%\n",
    (int)nation_id,
    decision->pct_military * 100.0,
    decision->pct_expansion * 100.0,
    decision->pct_defense * 100.0,
    decision->pct_economy * 100.0,
    decision->pct_scouting * 100.0);

  return 0;
}

static int
execute_attack(DECISION_PTR decision, PERSONALITY_PTR pers,
               BOARD_EVAL_PTR eval, FOW_PTR fog, ntntype nation_id)
{
  /* Sprint 0 stub: report strategy, no game actions */
  fprintf(stderr, "[AI] Nation %d ATTACK: (stub - not yet implemented)\n",
          (int)nation_id);
  return 0;
}

static int
execute_defend(DECISION_PTR decision, PERSONALITY_PTR pers,
               BOARD_EVAL_PTR eval, FOW_PTR fog, ntntype nation_id)
{
  fprintf(stderr, "[AI] Nation %d DEFEND: (stub - not yet implemented)\n",
          (int)nation_id);
  return 0;
}

static int
execute_economy(DECISION_PTR decision, PERSONALITY_PTR pers,
                BOARD_EVAL_PTR eval, FOW_PTR fog, ntntype nation_id)
{
  fprintf(stderr, "[AI] Nation %d ECONOMY: (stub - not yet implemented)\n",
          (int)nation_id);
  return 0;
}

static int
execute_diplomacy(DECISION_PTR decision, PERSONALITY_PTR pers,
                  BOARD_EVAL_PTR eval, FOW_PTR fog, ntntype nation_id)
{
  fprintf(stderr, "[AI] Nation %d DIPLOMACY: (stub - not yet implemented)\n",
          (int)nation_id);
  return 0;
}

static int
execute_scout(DECISION_PTR decision, PERSONALITY_PTR pers,
              BOARD_EVAL_PTR eval, FOW_PTR fog, ntntype nation_id)
{
  fprintf(stderr, "[AI] Nation %d SCOUT: (stub - not yet implemented)\n",
          (int)nation_id);
  return 0;
}

static int
execute_hybrid(DECISION_PTR decision, PERSONALITY_PTR pers,
               BOARD_EVAL_PTR eval, FOW_PTR fog, ntntype nation_id)
{
  fprintf(stderr, "[AI] Nation %d HYBRID: (stub - not yet implemented)\n",
          (int)nation_id);
  return 0;
}

int
execute_strategy(DECISION_PTR decision, PERSONALITY_PTR pers,
                 BOARD_EVAL_PTR eval, FOW_PTR fog, ntntype nation_id)
{
  if (!decision) return -1;

  switch (decision->strategy) {
    case AI_EXPAND:    return execute_expand(decision, pers, eval, fog, nation_id);
    case AI_ATTACK:   return execute_attack(decision, pers, eval, fog, nation_id);
    case AI_DEFEND:   return execute_defend(decision, pers, eval, fog, nation_id);
    case AI_ECONOMY:  return execute_economy(decision, pers, eval, fog, nation_id);
    case AI_DIPLOMACY: return execute_diplomacy(decision, pers, eval, fog, nation_id);
    case AI_SCOUT:    return execute_scout(decision, pers, eval, fog, nation_id);
    case AI_HYBRID:   return execute_hybrid(decision, pers, eval, fog, nation_id);
    default:
      fprintf(stderr, "[AI] Unknown strategy %d\n", decision->strategy);
      return -1;
  }
}

/* ------------------------------------------------------------------ */
/* Debug dumps                                                          */
/* ------------------------------------------------------------------ */

void
board_eval_dump(BOARD_EVAL_PTR eval)
{
  if (!eval) {
    fprintf(stderr, "board_eval_dump: NULL pointer\n");
    return;
  }

  fprintf(stderr, "=== Board Evaluation ===\n");
  fprintf(stderr, "  Territory: owned=%d visible=%d known=%d frontier=%d\n",
          eval->owned_sectors, eval->visible_sectors,
          eval->known_sectors, eval->frontier_sectors);
  fprintf(stderr, "  Military: own=%ld enemy_vis=%ld enemy_nations=%d\n",
          eval->total_military, eval->visible_enemy_military,
          eval->visible_enemy_nations);
  fprintf(stderr, "  Economy: pop=%ld eff=%.1f%% talons=%ld\n",
          eval->total_population, eval->avg_efficiency * 100.0,
          eval->total_talons);
  fprintf(stderr, "  Threat: level=%.0f%% threatened=%d hostile_borders=%d unknown_borders=%d\n",
          eval->threat_level * 100.0, eval->threatened_sectors,
          eval->bordering_hostile, eval->bordering_unknown);
  fprintf(stderr, "  Opportunity: expansion=%.0f%% unclaimed=%d weak=%d econ_pot=%.0f%%\n",
          eval->expansion_opportunity * 100.0, eval->unclaimed_adjacent,
          eval->weak_neighbor_sectors, eval->economic_potential * 100.0);
  fprintf(stderr, "  Diplomacy: allies=%d hostile=%d neutral=%d unmet=%d\n",
          eval->allied_nations, eval->hostile_nations,
          eval->neutral_nations, eval->unmet_nations);
}

void
decision_dump(DECISION_PTR decision)
{
  if (!decision) {
    fprintf(stderr, "decision_dump: NULL pointer\n");
    return;
  }

  fprintf(stderr, "=== Decision ===\n");
  fprintf(stderr, "  Strategy: %s (confidence %.0f%%)\n",
          ai_strategy_name[decision->strategy],
          decision->confidence * 100.0);
  fprintf(stderr, "  Allocation: mil=%.0f%% exp=%.0f%% def=%.0f%% eco=%.0f%% scout=%.0f%%\n",
          decision->pct_military * 100.0,
          decision->pct_expansion * 100.0,
          decision->pct_defense * 100.0,
          decision->pct_economy * 100.0,
          decision->pct_scouting * 100.0);
  fprintf(stderr, "  Effective priorities: mil=%.3f exp=%.3f def=%.3f eco=%.3f dip=%.3f scout=%.3f res=%.3f\n",
          decision->effective_priorities.military,
          decision->effective_priorities.expansion,
          decision->effective_priorities.defense,
          decision->effective_priorities.economy,
          decision->effective_priorities.diplomacy,
          decision->effective_priorities.scouting,
          decision->effective_priorities.research);
  fprintf(stderr, "  Reasoning: %s\n", decision->reasoning);
}