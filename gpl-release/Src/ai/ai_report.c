// SPDX-License-Identifier: GPL-3.0-or-later
/* ai_report.c - AI turn report generation and delivery */
/*
 * Conquer Reborn - Honest AI
 * Sprint 0, Task 0.5: Turn Report Generation
 *
 * After each AI nation takes its turn, this module produces a
 * human-readable report describing what the nation did, what it
 * saw, and why it chose its strategy.
 *
 * Reports are delivered through Conquer's existing mail system
 * (msg_cinit/msg_conquer/msg_cfinish), so they appear in the
 * player's in-game mail just like any other game event.
 *
 * Design principle: The report should read like a brief intelligence
 * briefing. No raw numbers dumped on the player — just the key
 * facts and the reasoning behind the AI's decisions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ai/ai_standalone_types.h"
#include "ai/ai_report.h"

/* ------------------------------------------------------------------ */
/* Action type names for reports                                       */
/* ------------------------------------------------------------------ */

static const char *action_type_name[] = {
  "Scout",		/* AI_ACT_SCOUT		*/
  "Expand",		/* AI_ACT_EXPAND	*/
  "Attack",		/* AI_ACT_ATTACK	*/
  "Defend",		/* AI_ACT_DEFEND	*/
  "Build",		/* AI_ACT_BUILD		*/
  "Diplomacy",		/* AI_ACT_DIPLOMACY	*/
  "Trade",		/* AI_ACT_TRADE		*/
  "Recon",		/* AI_ACT_RECON		*/
  "Loss",		/* AI_ACT_LOSS		*/
  "Other"		/* AI_ACT_OTHER		*/
};

/* ------------------------------------------------------------------ */
/* Report Initialization                                                */
/* ------------------------------------------------------------------ */

void
ai_report_init(AI_REPORT_PTR report, ntntype nation_id,
               int turn_number, const char *personality_name)
{
  if (report == NULL) return;

  memset(report, 0, sizeof(AI_REPORT_STRUCT));
  report->nation_id = nation_id;
  report->turn_number = turn_number;

  if (personality_name != NULL) {
    strncpy(report->personality_name, personality_name, NAMELTH - 1);
    report->personality_name[NAMELTH - 1] = '\0';
  }
}

/* ------------------------------------------------------------------ */
/* Action Recording                                                     */
/* ------------------------------------------------------------------ */

int
ai_report_add_action(AI_REPORT_PTR report, AIActionType type,
                     int x, int y, ntntype target_nation,
                     const char *description)
{
  if (report == NULL) return -1;

  if (report->action_count >= AI_REPORT_MAX_ACTIONS) {
    return -1;  /* action list full */
  }

  AI_ACTION_PTR action = &report->actions[report->action_count];
  action->type = type;
  action->x = x;
  action->y = y;
  action->target_nation = target_nation;

  if (description != NULL) {
    strncpy(action->description, description, BIGLTH - 1);
    action->description[BIGLTH - 1] = '\0';
  }

  report->action_count++;
  return 0;
}

/* ------------------------------------------------------------------ */
/* Strategy Info                                                        */
/* ------------------------------------------------------------------ */

void
ai_report_set_strategy(AI_REPORT_PTR report, DECISION_PTR decision)
{
  if (report == NULL || decision == NULL) return;

  report->strategy = decision->strategy;
  report->confidence = decision->confidence;
  strncpy(report->reasoning, decision->reasoning, BIGLTH - 1);
  report->reasoning[BIGLTH - 1] = '\0';
}

/* ------------------------------------------------------------------ */
/* Board State Snapshot                                                 */
/* ------------------------------------------------------------------ */

void
ai_report_set_board_state(AI_REPORT_PTR report, BOARD_EVAL_PTR eval)
{
  if (report == NULL || eval == NULL) return;

  report->sectors_owned = eval->owned_sectors;
  report->sectors_visible = eval->visible_sectors;
  report->sectors_known = eval->known_sectors;
  report->military_strength = eval->total_military;
  report->population = eval->total_population;
  report->avg_efficiency = eval->avg_efficiency;
  report->hostile_neighbors = eval->hostile_nations;
  report->allied_neighbors = eval->allied_nations;
}

/* ------------------------------------------------------------------ */
/* Report Formatting                                                    */
/* ------------------------------------------------------------------ */

int
ai_report_format(AI_REPORT_PTR report, char *buf, int bufsize)
{
  int pos = 0;
  int n;

  if (report == NULL || buf == NULL || bufsize < 1) return 0;

  /* Header */
  n = snprintf(buf + pos, bufsize - pos,
               "== %s Turn Report ==\n"
               "Turn %d | Strategy: %s (confidence: %.0f%%)\n"
               "Reasoning: %s\n\n",
               report->personality_name,
               report->turn_number,
               (report->strategy >= 0 && report->strategy < AI_STRATEGY_COUNT)
                 ? ai_strategy_name[report->strategy] : "UNKNOWN",
               report->confidence * 100.0,
               report->reasoning);
  if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;

  /* Board state summary */
  n = snprintf(buf + pos, bufsize - pos,
               "Territory: %d sectors owned, %d visible, %d known\n"
               "Military: %ld troops, Population: %ld\n"
               "Efficiency: %.0f%% | Hostile: %d | Allied: %d\n\n",
               report->sectors_owned,
               report->sectors_visible,
               report->sectors_known,
               report->military_strength,
               report->population,
               report->avg_efficiency,
               report->hostile_neighbors,
               report->allied_neighbors);
  if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;

  /* Outcome summary */
  if (report->sectors_scouted > 0 || report->sectors_expanded > 0 ||
      report->sectors_attacked > 0 || report->sectors_lost > 0 ||
      report->units_built > 0 || report->buildings_constructed > 0) {
    n = snprintf(buf + pos, bufsize - pos, "Results: ");
    if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;

    int first = 1;
    if (report->sectors_scouted > 0) {
      n = snprintf(buf + pos, bufsize - pos, "%sscouted %d sector%s",
                   first ? "" : ", ", report->sectors_scouted,
                   report->sectors_scouted == 1 ? "" : "s");
      if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;
      first = 0;
    }
    if (report->sectors_expanded > 0) {
      n = snprintf(buf + pos, bufsize - pos, "%sexpanded into %d sector%s",
                   first ? "" : ", ", report->sectors_expanded,
                   report->sectors_expanded == 1 ? "" : "s");
      if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;
      first = 0;
    }
    if (report->sectors_attacked > 0) {
      n = snprintf(buf + pos, bufsize - pos, "%sattacked %d sector%s",
                   first ? "" : ", ", report->sectors_attacked,
                   report->sectors_attacked == 1 ? "" : "s");
      if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;
      first = 0;
    }
    if (report->sectors_lost > 0) {
      n = snprintf(buf + pos, bufsize - pos, "%slost %d sector%s",
                   first ? "" : ", ", report->sectors_lost,
                   report->sectors_lost == 1 ? "" : "s");
      if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;
      first = 0;
    }
    if (report->units_built > 0) {
      n = snprintf(buf + pos, bufsize - pos, "%sbuilt %d unit%s",
                   first ? "" : ", ", report->units_built,
                   report->units_built == 1 ? "" : "s");
      if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;
      first = 0;
    }
    if (report->buildings_constructed > 0) {
      n = snprintf(buf + pos, bufsize - pos, "%sconstructed %d building%s",
                   first ? "" : ", ", report->buildings_constructed,
                   report->buildings_constructed == 1 ? "" : "s");
      if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;
      first = 0;
    }
    n = snprintf(buf + pos, bufsize - pos, ".\n\n");
    if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;
  }

  /* Action log */
  if (report->action_count > 0) {
    n = snprintf(buf + pos, bufsize - pos, "Action Log:\n");
    if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;

    int i;
    for (i = 0; i < report->action_count && i < AI_REPORT_MAX_ACTIONS; i++) {
      AI_ACTION_PTR a = &report->actions[i];
      const char *type_str = (a->type >= 0 && a->type <= AI_ACT_OTHER)
                              ? action_type_name[a->type] : "Unknown";

      if (a->x >= 0 && a->y >= 0) {
        n = snprintf(buf + pos, bufsize - pos,
                     "  [%s] [%d,%d] %s\n",
                     type_str, a->x, a->y, a->description);
      } else {
        n = snprintf(buf + pos, bufsize - pos,
                     "  [%s] %s\n",
                     type_str, a->description);
      }
      if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;
    }
  }

  /* Footer */
  n = snprintf(buf + pos, bufsize - pos,
               "\n-- End %s Report --\n",
               report->personality_name);
  if (n > 0) pos += (pos + n < bufsize) ? n : bufsize - pos - 1;

  return pos;
}

/* ------------------------------------------------------------------ */
/* Report Delivery (requires Conquer mail system)                      */
/* ------------------------------------------------------------------ */

/* When building with Conquer, use the real mail system.
 * For standalone testing, this function is a no-op stub.
 */
#ifdef MEMORYH

/* Deliver through Conquer's mail system */
int
ai_report_deliver(AI_REPORT_PTR report)
{
  extern void msg_ginit(char *str);
  extern void msg_grouped(ntntype to, int xloc, int yloc, char *msgstr);
  extern void msg_gfinish(void);

  char buf[4096];
  int len;

  if (report == NULL) return -1;

  /* Format the report */
  len = ai_report_format(report, buf, sizeof(buf));
  if (len <= 0) return -1;

  /* Initialize a grouped mail message */
  msg_ginit("AI Turn Report");

  /* In observation mode: deliver to all player nations.
   * In human-vs-AI mode: deliver only to allied/friendly nations.
   * Sprint 0: deliver to all (observation mode).
   *
   * TODO: Add visibility filtering for human-vs-AI mode.
   *   - All reports visible in observation mode
   *   - Only allied/friendly reports in human-vs-AI mode
   *   - Intercepted intel possible if scouts are nearby
   */

  /* Deliver the formatted report.
   * Use coordinate -1, -1 to indicate no specific location.
   */
  /* Deliver to the nation itself (always) */
  msg_grouped(report->nation_id, -1, -1, buf);

  /* Close the grouped message */
  msg_gfinish();

  return 0;
}

#else  /* !MEMORYH - standalone testing */

int
ai_report_deliver(AI_REPORT_PTR report)
{
  /* Standalone stub - no mail system available */
  (void)report;
  return 0;
}

#endif /* MEMORYH */

/* ------------------------------------------------------------------ */
/* Debug Dump                                                           */
/* ------------------------------------------------------------------ */

void
ai_report_dump(AI_REPORT_PTR report)
{
  int i;

  if (report == NULL) {
    fprintf(stderr, "  [AI Report] NULL\n");
    return;
  }

  fprintf(stderr, "  [AI Report] Nation %d (%s) Turn %d\n",
          (int)report->nation_id, report->personality_name,
          report->turn_number);
  fprintf(stderr, "  [AI Report] Strategy: %s (confidence %.0f%%)\n",
          (report->strategy >= 0 && report->strategy < AI_STRATEGY_COUNT)
            ? ai_strategy_name[report->strategy] : "UNKNOWN",
          report->confidence * 100.0);
  fprintf(stderr, "  [AI Report] Reasoning: %s\n", report->reasoning);
  fprintf(stderr, "  [AI Report] Sectors: %d owned, %d visible, %d known\n",
          report->sectors_owned, report->sectors_visible,
          report->sectors_known);
  fprintf(stderr, "  [AI Report] Military: %ld, Population: %ld\n",
          report->military_strength, report->population);
  fprintf(stderr, "  [AI Report] Hostile: %d, Allied: %d\n",
          report->hostile_neighbors, report->allied_neighbors);
  fprintf(stderr, "  [AI Report] Scouted: %d, Expanded: %d, Attacked: %d, Lost: %d\n",
          report->sectors_scouted, report->sectors_expanded,
          report->sectors_attacked, report->sectors_lost);
  fprintf(stderr, "  [AI Report] Units built: %d, Buildings: %d\n",
          report->units_built, report->buildings_constructed);
  fprintf(stderr, "  [AI Report] Actions: %d\n", report->action_count);

  for (i = 0; i < report->action_count && i < AI_REPORT_MAX_ACTIONS; i++) {
    AI_ACTION_PTR a = &report->actions[i];
    const char *type_str = (a->type >= 0 && a->type <= AI_ACT_OTHER)
                            ? action_type_name[a->type] : "Unknown";
    fprintf(stderr, "  [AI Report]   %s [%d,%d] target=%d: %s\n",
            type_str, a->x, a->y, (int)a->target_nation, a->description);
  }
}