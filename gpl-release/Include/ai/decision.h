// SPDX-License-Identifier: GPL-3.0-or-later
/* decision.h - AI decision engine data structures and API */
/*
 * Conquer Reborn - Honest AI
 *
 * The decision engine evaluates board state through a nation's fog of war,
 * blends situational weights with personality priorities, and selects
 * a strategy. Then it executes that strategy through action functions.
 *
 * Sprint 0: Warlord proof of concept with EXPAND strategy fully wired.
 * Other strategies route to stubs.
 */

#ifndef DECISION_H
#define DECISION_H

/* ntntype must be defined before including this header.
 * When building with Conquer: sysconf.h defines it.
 * For standalone testing: provide a typedef.
 */

#include "ai/personality.h"
#include "ai/fog_of_war.h"

/* ------------------------------------------------------------------ */
/* Strategy Types                                                      */
/* ------------------------------------------------------------------ */

typedef enum {
  AI_EXPAND,		/* Aggressive territorial expansion	*/
  AI_ATTACK,		/* Military offensive against a target	*/
  AI_DEFEND,		/* Fortify and hold current territory	*/
  AI_ECONOMY,		/* Focus on production and trade		*/
  AI_DIPLOMACY,		/* Pursue alliances and treaties		*/
  AI_SCOUT,		/* Prioritize information gathering	*/
  AI_HYBRID,		/* Blended strategy (fallback)		*/
  AI_STRATEGY_COUNT	/* total number of strategies		*/
} AIStrategy;

/* Strategy names for debugging and reports */
extern const char *ai_strategy_name[];

/* ------------------------------------------------------------------ */
/* Board Evaluation - what the AI can see and deduce                   */
/* ------------------------------------------------------------------ */

typedef struct s_board_eval {
  /* Territory assessment */
  int owned_sectors;		/* sectors this nation owns			*/
  int visible_sectors;		/* sectors currently visible			*/
  int known_sectors;		/* sectors ever seen				*/
  int frontier_sectors;		/* owned sectors bordering unowned/enemy	*/

  /* Military assessment */
  long total_military;		/* own military strength			*/
  long visible_enemy_military;	/* enemy military in visible sectors		*/
  int visible_enemy_nations;	/* number of distinct enemy nations visible	*/

  /* Economic assessment */
  long total_population;	/* own civilian population			*/
  double avg_efficiency;	/* average sector efficiency			*/
  long total_talons;		/* treasury (estimated)				*/

  /* Threat assessment */
  double threat_level;		/* 0.0 (no threats) to 1.0 (imminent danger)	*/
  int threatened_sectors;	/* sectors under immediate threat		*/
  int bordering_hostile;		/* sectors bordering hostile nations		*/
  int bordering_unknown;	/* sectors bordering unexplored territory	*/

  /* Opportunity assessment */
  double expansion_opportunity;	/* 0.0 (no room) to 1.0 (wide open)		*/
  int unclaimed_adjacent;	/* unowned sectors adjacent to territory	*/
  int weak_neighbor_sectors;	/* enemy sectors with low military nearby	*/
  double economic_potential;	/* 0.0 (poor) to 1.0 (rich)			*/

  /* Diplomatic state */
  int allied_nations;		/* number of allies				*/
  int hostile_nations;		/* number of nations at war/hostile		*/
  int neutral_nations;		/* number of neutral nations contacted		*/
  int unmet_nations;		/* nations not yet encountered			*/
} BOARD_EVAL_STRUCT, *BOARD_EVAL_PTR;

/* ------------------------------------------------------------------ */
/* Strategy Decision - the chosen strategy and its context             */
/* ------------------------------------------------------------------ */

typedef struct s_decision {
  AIStrategy strategy;		/* chosen strategy				*/
  double confidence;		/* how confident in this choice (0.0-1.0)	*/

  /* Effective priorities used for this decision (blended) */
  PRIORITY_STRUCT effective_priorities;

  /* Strategy-specific parameters */
  int primary_target_x;		/* x coord of primary action target		*/
  int primary_target_y;		/* y coord of primary action target		*/
  int secondary_target_x;	/* x coord of secondary target			*/
  int secondary_target_y;	/* y coord of secondary target			*/

  /* Resource allocation for this turn (percentages) */
  double pct_military;		/* % of resources to military actions		*/
  double pct_expansion;		/* % of resources to expansion actions		*/
  double pct_defense;		/* % of resources to defensive actions		*/
  double pct_economy;		/* % of resources to economic actions		*/
  double pct_scouting;		/* % of resources to scouting actions		*/

  /* Reasoning for reports */
  char reasoning[BIGLTH];	/* human-readable explanation			*/
} DECISION_STRUCT, *DECISION_PTR;

/* ------------------------------------------------------------------ */
/* API Functions                                                       */
/* ------------------------------------------------------------------ */

/* Evaluate the board state through a nation's fog of war.
 * Reads world data (through fog) and populates the board evaluation.
 * Returns 0 on success, -1 on error.
 */
int evaluate_board(BOARD_EVAL_PTR eval, FOW_PTR fog,
                   PERSONALITY_PTR pers, ntntype nation_id);

/* Select the best strategy based on personality and board evaluation.
 * Blends base personality priorities with situational weights
 * using the adaptation formula, then picks the highest-weighted strategy.
 * Populates the decision struct with the chosen strategy and parameters.
 * Returns 0 on success, -1 on error.
 */
int select_strategy(DECISION_PTR decision, PERSONALITY_PTR pers,
                    BOARD_EVAL_PTR eval, FOW_PTR fog);

/* Execute a strategy for one AI nation turn.
 * Routes to the appropriate action functions based on strategy type.
 * Sprint 0: Only AI_EXPAND is fully implemented; others are stubs.
 * Returns 0 on success, -1 on error.
 */
int execute_strategy(DECISION_PTR decision, PERSONALITY_PTR pers,
                     BOARD_EVAL_PTR eval, FOW_PTR fog, ntntype nation_id);

/* Compute situational weights from board evaluation.
 * Public so tests can verify the blending formula.
 */
void compute_situational_weights(BOARD_EVAL_PTR eval, PRIORITY_PTR situational);

/* Dump board evaluation to stderr for debugging */
void board_eval_dump(BOARD_EVAL_PTR eval);

/* Dump decision to stderr for debugging */
void decision_dump(DECISION_PTR decision);

#endif /* DECISION_H */