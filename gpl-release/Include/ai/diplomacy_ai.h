// SPDX-License-Identifier: GPL-3.0-or-later
/* diplomacy_ai.h - AI diplomacy module */
/*
 * Conquer Reborn - Honest AI
 * Sprint 1, Task 1.3: AI Diplomacy Module
 *
 * Personality-driven diplomacy between AI nations. Each AI nation
 * evaluates its known neighbors through fog of war, builds trust
 * levels, and decides diplomatic actions based on personality weights.
 *
 * Key principle: AI uses NO information it can't see. Diplomatic
 * decisions are based on fog-of-war-filtered perception, just like
 * a human player would make them.
 */

#ifndef DIPLOMACY_AI_H
#define DIPLOMACY_AI_H

#include "ai/ai_standalone_types.h"

/* ------------------------------------------------------------------ */
/* Trust model                                                          */
/* ------------------------------------------------------------------ */

/* Per-nation trust and diplomatic assessment.
 * One DIPLO_ASSESS per (self, other) pair.
 */
typedef struct s_diplo_assess {
  ntntype other_nation;		/* Who we're assessing		*/
  int     known;		/* 1 if we've ever seen them	*/

  /* Trust (0.0 - 1.0) */
  double  trust;		/* Current trust level		*/
  double  perceived_strength;	/* Estimated military power	*/
  double  perceived_threat;	/* How threatening they seem	*/
  double  perceived_economy;	/* Estimated economic power	*/

  /* What we know about them (fog-filtered) */
  int     visible_sectors_near_border;  /* Sectors of theirs we can see */
  int     border_incidents;		    /* Times they crossed our land */
  int     diplomatic_events;		    /* Number of diplomatic interactions */

  /* Last diplomatic action we took toward them */
  int     last_action_turn;	/* Turn of last action		*/
  int     last_action_type;	/* DIPLO_ACT_* type, 0=none	*/
} DIPLO_ASSESS_STRUCT, *DIPLO_ASSESS_PTR;

/* ------------------------------------------------------------------ */
/* Diplomatic action types                                               */
/* ------------------------------------------------------------------ */

typedef enum diplotype_action {
  DIPLO_ACT_NONE = 0,		/* No action taken		*/
  DIPLO_ACT_PROPOSE_ALLIANCE,	/* Seek alliance		*/
  DIPLO_ACT_PROPOSE_TREATY,	/* Seek treaty			*/
  DIPLO_ACT_PROPOSE_TRADE,	/* Open trade relations		*/
  DIPLO_ACT_IMPROVE_RELATIONS,	/* Send diplomatic envoy	*/
  DIPLO_ACT_WARN,		/* Issue warning		*/
  DIPLO_ACT_DEMAND,		/* Make demand			*/
  DIPLO_ACT_DECLARE_WAR,	/* Declare war			*/
  DIPLO_ACT_REQUEST_PEACE,	/* Request peace		*/
  DIPLO_ACT_BREAK_TREATY,	/* Break existing treaty	*/
  DIPLO_ACT_SHARE_INTEL,	/* Share map data with ally	*/
  DIPLO_ACT_COUNT		/* Number of action types	*/
} DiplomacyAction;

/* ------------------------------------------------------------------ */
/* Diplomacy module state                                                */
/* ------------------------------------------------------------------ */

#define DIPLO_MAX_ASSESSMENTS 100  /* Max nations we track */

typedef struct s_diplomacy_state {
  ntntype           self_nation;	/* Our nation ID		*/
  DIPLO_ASSESS_STRUCT assessments[DIPLO_MAX_ASSESSMENTS];
  int               assessment_count;

  /* Aggregate metrics */
  int     allied_count;		/* Current allies			*/
  int     hostile_count;	/* Current hostile nations		*/
  int     neutral_count;	/* Nations we haven't met		*/

  /* Decision output: what we want to do this turn */
  DiplomacyAction proposed_action;	/* Top priority action	*/
  ntntype         proposed_target;	/* Target nation		*/
  char            proposed_reason[BIGLTH];	/* Why		*/
  double          proposed_confidence;	/* How confident (0-1)	*/
} DIPLOMACY_STATE_STRUCT, *DIPLOMACY_STATE_PTR;

/* ------------------------------------------------------------------ */
/* API Functions                                                        */
/* ------------------------------------------------------------------ */

/* Initialize diplomacy state for a nation */
void diplomacy_init(DIPLOMACY_STATE_PTR state, ntntype nation_id);

/* Evaluate all known nations and update assessments.
 * Uses fog-of-war-filtered data (no omniscience).
 * Returns number of nations assessed, or -1 on error.
 */
int diplomacy_evaluate(DIPLOMACY_STATE_PTR state,
                       PERSONALITY_PTR pers,
                       FOW_PTR fog,
                       ntntype nation_id);

/* Determine what diplomatic action to take this turn.
 * Populates state->proposed_action/target/reason.
 * Returns 0 if an action is proposed, 1 if no action needed.
 */
int diplomacy_propose(DIPLOMACY_STATE_PTR state,
                      PERSONALITY_PTR pers,
                      BOARD_EVAL_PTR eval);

/* Share map data with an ally.
 * sharing_level: 0.0-1.0, how much of our map to reveal.
 * Allied = 0.7, Treaty = 0.4, Friendly = 0.2
 */
void diplomacy_share_intel(DIPLOMACY_STATE_PTR state,
                           FOW_PTR our_fog,
                           ntntype ally_nation,
                           double sharing_level);

/* Decay trust and reset transient state between turns.
 * Call at the start of each AI turn.
 */
void diplomacy_turn_update(DIPLOMACY_STATE_PTR state,
                           PERSONALITY_PTR pers);

/* Get assessment for a specific nation.
 * Returns pointer into state, or NULL if not found.
 */
DIPLO_ASSESS_PTR diplomacy_get_assess(DIPLOMACY_STATE_PTR state,
                                      ntntype other_nation);

/* Get the recommended diplomatic stance toward a nation.
 * Combines trust level, personality, and current situation.
 * Returns a Diplotype value (DIP_UNMET through DIP_JIHAD).
 */
int diplomacy_recommended_stance(DIPLOMACY_STATE_PTR state,
                                  PERSONALITY_PTR pers,
                                  ntntype other_nation);

/* Dump diplomacy state to stderr for debugging */
void diplomacy_dump(DIPLOMACY_STATE_PTR state);

#endif /* DIPLOMACY_AI_H */