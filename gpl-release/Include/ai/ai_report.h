// SPDX-License-Identifier: GPL-3.0-or-later
/* ai_report.h - AI turn report data structures and API */
/*
 * Conquer Reborn - Honest AI
 * Sprint 0, Task 0.5: Turn Report Generation
 *
 * After each AI nation takes its turn, a report is generated
 * describing what the nation did, what it saw, and why it chose
 * its strategy. In observation mode, all reports are visible.
 * In human-vs-AI mode, only allied/friendly reports are visible.
 *
 * The report is delivered through Conquer's existing mail system
 * (msg_cinit/msg_conquer/msg_cfinish).
 */

#ifndef AI_REPORT_H
#define AI_REPORT_H

/* ntntype must be defined before including this header.
 * When building with Conquer: sysconf.h defines it.
 * For standalone testing: provide a typedef via ai_standalone_types.h.
 */

#include "ai/personality.h"
#include "ai/decision.h"

/* ------------------------------------------------------------------ */
/* Report Structure                                                    */
/* ------------------------------------------------------------------ */

/* Maximum number of action entries in a single turn report */
#define AI_REPORT_MAX_ACTIONS 32

/* Action types that can appear in a turn report */
typedef enum {
  AI_ACT_SCOUT,		/* Scouted a sector				*/
  AI_ACT_EXPAND,	/* Expanded into a sector			*/
  AI_ACT_ATTACK,	/* Attacked an enemy sector			*/
  AI_ACT_DEFEND,	/* Fortified a sector				*/
  AI_ACT_BUILD,		/* Built units or improvements			*/
  AI_ACT_DIPLOMACY,	/* Diplomatic action (offer, respond)		*/
  AI_ACT_TRADE,		/* Trade or economic action			*/
  AI_ACT_RECON,		/* Reconnaissance report (what was seen)		*/
  AI_ACT_LOSS,		/* Lost a sector or unit			*/
  AI_ACT_OTHER		/* Miscellaneous action				*/
} AIActionType;

/* A single action in a turn report */
typedef struct s_ai_action {
  AIActionType type;		/* what kind of action			*/
  int x, y;			/* sector coordinates (or -1 if N/A)	*/
  ntntype target_nation;		/* target nation (or UNOWNED if N/A)	*/
  char description[BIGLTH];	/* human-readable description		*/
} AI_ACTION_STRUCT, *AI_ACTION_PTR;

/* Full turn report for one AI nation */
typedef struct s_ai_report {
  /* Identification */
  ntntype nation_id;		/* which nation this report is for	*/
  int turn_number;		/* game turn number			*/
  char nation_name[NAMELTH];	/* nation name (for readability)	*/
  char personality_name[NAMELTH]; /* personality name (e.g. "Warlord")	*/

  /* Strategy summary */
  AIStrategy strategy;		/* the strategy chosen this turn	*/
  double confidence;		/* confidence in the strategy (0.0-1.0)	*/
  char reasoning[BIGLTH];	/* why this strategy was chosen		*/

  /* Board state snapshot (what the AI could see) */
  int sectors_owned;		/* sectors this nation owns		*/
  int sectors_visible;		/* sectors visible this turn		*/
  int sectors_known;		/* sectors ever seen			*/
  long military_strength;	/* own military strength		*/
  long population;		/* own civilian population		*/
  double avg_efficiency;		/* average sector efficiency		*/
  int hostile_neighbors;	/* nations with hostile/war status	*/
  int allied_neighbors;		/* allied nations			*/

  /* Actions taken this turn */
  int action_count;		/* number of actions recorded		*/
  AI_ACTION_STRUCT actions[AI_REPORT_MAX_ACTIONS];

  /* Outcome summary */
  int sectors_scouted;		/* sectors newly revealed		*/
  int sectors_attacked;		/* enemy sectors attacked		*/
  int sectors_expanded;		/* new sectors claimed			*/
  int sectors_lost;		/* sectors lost to enemies		*/
  int units_built;		/* military units built			*/
  int buildings_constructed;	/* buildings constructed		*/
} AI_REPORT_STRUCT, *AI_REPORT_PTR;

/* ------------------------------------------------------------------ */
/* API Functions                                                       */
/* ------------------------------------------------------------------ */

/* Initialize a turn report for a nation.
 * Sets nation_id, turn, personality, and zeroes all counters.
 */
void ai_report_init(AI_REPORT_PTR report, ntntype nation_id,
                    int turn_number, const char *personality_name);

/* Record an action in the turn report.
 * Returns 0 on success, -1 if action list is full.
 */
int ai_report_add_action(AI_REPORT_PTR report, AIActionType type,
                         int x, int y, ntntype target_nation,
                         const char *description);

/* Finalize the report: set strategy info from the decision struct.
 * Call this after the strategy is chosen and executed.
 */
void ai_report_set_strategy(AI_REPORT_PTR report, DECISION_PTR decision);

/* Set board state snapshot in the report from board evaluation.
 * Call this after evaluate_board() and before strategy execution.
 */
void ai_report_set_board_state(AI_REPORT_PTR report, BOARD_EVAL_PTR eval);

/* Generate a human-readable turn report string.
 * Writes formatted report into the provided buffer.
 * Returns the number of characters written (excluding null terminator).
 */
int ai_report_format(AI_REPORT_PTR report, char *buf, int bufsize);

/* Deliver the report through Conquer's mail system.
 * In observation mode: report goes to all players.
 * In human-vs-AI mode: report goes to allied/friendly nations only.
 * The nation's own report is always delivered to itself.
 * Returns 0 on success, -1 on failure.
 */
int ai_report_deliver(AI_REPORT_PTR report);

/* Dump report to stderr for debugging */
void ai_report_dump(AI_REPORT_PTR report);

#endif /* AI_REPORT_H */