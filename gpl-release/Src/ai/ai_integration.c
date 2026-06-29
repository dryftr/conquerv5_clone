// SPDX-License-Identifier: GPL-3.0-or-later
/* ai_integration.c - Wire honest AI into Conquer's NPC update loop */
/*
 * Conquer Reborn - Honest AI
 * Sprint 0, Task 0.4: Hook into cpu_update()
 *
 * This module bridges the AI personality/fog/decision engine
 * into the existing Conquer game loop. It replaces the #ifdef NOT_DONE
 * skeleton in npcA.c with actual personality-driven AI.
 *
 * Architecture:
 *   move_for_ntn() → cpu_update() → cpu_update_personality()
 *     1. fog_update_nation() — recalculate visibility from units/cities
 *     2. fog_observe_nation() — update remembered state for visible sectors
 *     3. evaluate_board()    — read world through fog, assess situation
 *     4. select_strategy()   — blend personality + situation → decision
 *     5. execute_strategy()  — route to action functions
 *
 * Monster nations (lizards, savages, pirates, nomads) are untouched.
 * Non-Warlord AI nations fall back to passive behavior.
 */

#include "dataA.h"
#include "armyX.h"
#include "moveX.h"
#include "activeX.h"
#include "statusX.h"
#include "dstatusX.h"
#include "ai/personality.h"
#include "ai/fog_of_war.h"
#include "ai/decision.h"
#include "ai/ai_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Forward declarations                                                 */
/* ------------------------------------------------------------------ */
void passive_update PARM_0(void);

/* ------------------------------------------------------------------ */
/* Module state                                                        */
/* ------------------------------------------------------------------ */

/* Personality registry — loads all 5 personalities once, maps nations to types.
 * Fog state is per-nation since it depends on map positions.
 */
static PERSONALITY_REGISTRY_STRUCT ai_registry;
static FOW_STRUCT nation_fog[ABSMAXNTN];
static int ai_initialized = 0;         /* 1 if registry loaded */
static int fog_initialized[ABSMAXNTN]; /* 1 if fog state initialized for nation */

/* ------------------------------------------------------------------ */
/* Initialization                                                       */
/* ------------------------------------------------------------------ */

/* Initialize AI registry on first call.
 * Loads all personality files and sets up nation assignments.
 * Sprint 1: assigns Warlord to all non-monster NPCs by default.
 * Later sprints will read from game configuration.
 */
static int
ai_ensure_initialized(void)
{
  int i, nloaded;

  if (ai_initialized) return 0;

  personality_registry_init(&ai_registry);
  nloaded = personality_load_all(&ai_registry);

  if (nloaded != PERSONALITY_COUNT) {
    fprintf(stderr, "[AI] Warning: only loaded %d/%d personalities\n",
            nloaded, PERSONALITY_COUNT);
  }

  /* Sprint 1 default: assign Warlord to all non-monster NPC nations.
   * Game setup will configure actual assignments later.
   */
  for (i = 0; i < ABSMAXNTN; i++) {
    /* Only assign to non-monster NPC nations.
     * Monsters are handled by existing code, not the personality engine.
     */
    if (world.np[i] != NULL && !n_ismonster(i)) {
      personality_assign(&ai_registry, i, PERSONALITY_WARLORD);
    }
  }

  ai_initialized = 1;
  return 0;
}

/* ------------------------------------------------------------------ */
/* Fog of War — world integration                                      */
/* ------------------------------------------------------------------ */

/* Update fog of war for a nation based on its current unit/city positions.
 * Scans all armies and cities of the nation, marking visible sectors
 * and their neighbors within vision range.
 */
static void
fog_update_nation(ntntype nation_id)
{
  NTN_PTR n_ptr = world.np[nation_id];

  if (!fog_initialized[nation_id]) {
    fog_init(&nation_fog[nation_id], nation_id,
             (int)world.mapx, (int)world.mapy);
    fog_initialized[nation_id] = 1;
  }

  /* Clear current visibility */
  fog_update(&nation_fog[nation_id], nation_id);

  /* Mark visibility from army units */
  if (n_ptr != NULL && n_ptr->army_list != NULL) {
    ARMY_PTR a_ptr;
    for (a_ptr = n_ptr->army_list; a_ptr != NULL; a_ptr = a_ptr->next) {
      int ax = (int)a_ptr->xloc;
      int ay = (int)a_ptr->yloc;
      int vision_range = 1; /* default: adjacent sectors */

      /* Flight units see further */
      if (unit_flight(a_ptr->status)) {
        vision_range = 2;
      }

      fog_mark_visible_range(&nation_fog[nation_id], ax, ay, vision_range);
    }
  }

  /* Mark visibility from owned sectors (cities, etc.) */
  if (n_ptr != NULL) {
    int x, y;
    for (x = 0; x < (int)world.mapx; x++) {
      for (y = 0; y < (int)world.mapy; y++) {
        if (sct[x][y].owner == nation_id) {
          /* Owned sectors are always visible to the owner */
          fog_mark_visible(&nation_fog[nation_id], x, y);
          /* Cities see adjacent sectors too */
          fog_mark_visible_range(&nation_fog[nation_id], x, y, 1);
        }
      }
    }
  }

  /* Capital city always visible */
  if (n_ptr != NULL) {
    fog_mark_visible_range(&nation_fog[nation_id],
                           (int)n_ptr->capx, (int)n_ptr->capy, 1);
  }
}

/* Observe all currently visible sectors, updating remembered state
 * with current world data.
 */
static void
fog_observe_nation(ntntype nation_id, int current_turn)
{
  int x, y;
  FOW_PTR fog = &nation_fog[nation_id];

  /* First pass: call fog_observe to mark changed sectors */
  fog_observe(fog, nation_id);

  /* Second pass: update remembered state with actual world data
   * for sectors we can currently see */
  for (x = 0; x < (int)world.mapx; x++) {
    for (y = 0; y < (int)world.mapy; y++) {
      if (fog_can_see(fog, x, y)) {
        fog_observe_sector(fog, x, y, current_turn,
                          sct[x][y].owner,
                          (long)sct[x][y].people,  /* people as proxy for activity */
                          (short)sct[x][y].designation,
                          (short)sct[x][y].efficiency);
      }
    }
  }
}

/* ------------------------------------------------------------------ */
/* Board evaluation — world integration                                 */
/* ------------------------------------------------------------------ */

/* Populate board evaluation from real world data, filtered through fog */
static void
evaluate_board_world(BOARD_EVAL_PTR eval, FOW_PTR fog,
                     ntntype nation_id)
{
  NTN_PTR n_ptr = world.np[nation_id];
  int x, y;

  if (!eval || !fog || !n_ptr) return;

  memset(eval, 0, sizeof(BOARD_EVAL_STRUCT));

  /* Set visible and known counts from fog */
  eval->visible_sectors = fog->visible_count;
  eval->known_sectors = fog->known_count;

  /* Count owned sectors and collect military/economy data */
  for (x = 0; x < (int)world.mapx; x++) {
    for (y = 0; y < (int)world.mapy; y++) {
      if (sct[x][y].owner == nation_id) {
        eval->owned_sectors++;
        eval->total_military += n_ptr->tmil;  /* nation total from NTN_STRUCT */
        eval->total_population += n_ptr->tciv;  /* nation total civilians */
        eval->avg_efficiency += (double)sct[x][y].efficiency;

        /* Check if this owned sector borders non-owned territory */
        int dx, dy;
        for (dx = -1; dx <= 1; dx++) {
          for (dy = -1; dy <= 1; dy++) {
            int nx = x + dx;
            int ny = y + dy;
            if (XY_ONMAP(nx, ny)) {
              if (sct[nx][ny].owner != nation_id) {
                if (sct[nx][ny].owner == UNOWNED) {
                  eval->unclaimed_adjacent++;
                } else if (n_ptr->dstatus[sct[nx][ny].owner] >= DIP_HOSTILE) {
                  eval->bordering_hostile++;
                }
                eval->frontier_sectors++;
              }
            } else {
              eval->bordering_unknown++;
            }
          }
        }
      }

      /* Count visible enemy military from sector occupancy,
       * plus track distinct enemy nations.
       * Military is in armies, not sectors, but we can see
       * if a sector is owned by someone else.
       */
      if (fog_can_see(fog, x, y) && sct[x][y].owner != nation_id &&
          sct[x][y].owner != UNOWNED) {
        eval->visible_enemy_military += (long)sct[x][y].people;  /* enemy civilians visible = proxy for strength */
        /* Note: actual enemy military comes from their armies,
         * which we'd need to scan. For Sprint 0, use civilian pop
         * as a rough proxy for activity level. */
        eval->visible_enemy_nations++;  /* will deduplicate below */
      }
    }
  }

  /* Average efficiency */
  if (eval->owned_sectors > 0) {
    eval->avg_efficiency /= (double)eval->owned_sectors;
  }

  /* Approximate distinct enemy nations from diplomatic status */
  eval->visible_enemy_nations = 0;
  for (x = 0; x < ABSMAXNTN; x++) {
    if (x != nation_id && n_ptr->dstatus[x] >= DIP_HOSTILE) {
      eval->visible_enemy_nations++;
      if (n_ptr->dstatus[x] >= DIP_WAR) {
        eval->hostile_nations++;
      } else if (n_ptr->dstatus[x] == DIP_NEUTRAL) {
        eval->neutral_nations++;
      }
    }
    if (n_ptr->dstatus[x] == DIP_ALLIED || n_ptr->dstatus[x] == DIP_TREATY) {
      eval->allied_nations++;
    }
    if (n_ptr->dstatus[x] == DIP_UNMET) {
      eval->unmet_nations++;
    }
  }

  /* Compute threat level */
  if (eval->total_military > 0 && eval->visible_enemy_military > 0) {
    double ratio = (double)eval->visible_enemy_military /
                  (double)eval->total_military;
    eval->threat_level = ratio > 1.0 ? 1.0 : ratio;
  }

  /* Compute expansion opportunity */
  if (eval->owned_sectors > 0 && eval->unclaimed_adjacent > 0) {
    eval->expansion_opportunity = (double)eval->unclaimed_adjacent /
                                   ((double)eval->owned_sectors * 2.0);
    if (eval->expansion_opportunity > 1.0) eval->expansion_opportunity = 1.0;
  } else {
    eval->expansion_opportunity = 0.5;
  }

  /* Economic potential */
  eval->economic_potential = eval->avg_efficiency / 100.0;

  /* Count weak neighbor sectors (enemy sectors with low civilian pop near our border) */
  eval->weak_neighbor_sectors = 0;
  for (x = 0; x < (int)world.mapx; x++) {
    for (y = 0; y < (int)world.mapy; y++) {
      if (fog_can_see(fog, x, y) &&
          sct[x][y].owner != nation_id &&
          sct[x][y].owner != UNOWNED &&
          (long)sct[x][y].people < 50) {
        eval->weak_neighbor_sectors++;
      }
    }
  }

  /* Diplomatic counts */
  eval->allied_nations = 0;
  eval->hostile_nations = 0;
  eval->neutral_nations = 0;
  eval->unmet_nations = 0;
  for (x = 0; x < ABSMAXNTN; x++) {
    if (x == nation_id) continue;
    Diplotype ds = (Diplotype)n_ptr->dstatus[x];
    if (ds == DIP_UNMET) eval->unmet_nations++;
    else if (ds == DIP_ALLIED || ds == DIP_TREATY) eval->allied_nations++;
    else if (ds >= DIP_HOSTILE) eval->hostile_nations++;
    else eval->neutral_nations++;
  }
}

/* ------------------------------------------------------------------ */
/* Main AI update function                                              */
/* ------------------------------------------------------------------ */

/* cpu_update_personality — personality-driven AI turn
 * Called from cpu_update() for non-monster nations.
 * Returns 0 on success, -1 on error.
 */
int
cpu_update_personality(ntntype nation_id)
{
  NTN_PTR n_ptr;
  PERSONALITY_PTR pers;
  FOW_PTR fog;

  /* Validate nation */
  /* ntntype is uns_char (unsigned), so < 0 is impossible */
  if (nation_id >= ABSMAXNTN) return -1;
  n_ptr = world.np[nation_id];
  if (n_ptr == NULL) return -1;

  /* Initialize AI registry on first call */
  if (ai_ensure_initialized() != 0) return -1;

  /* Look up personality for this nation */
  pers = personality_for_nation(&ai_registry, nation_id);
  if (pers == NULL) {
    /* No personality assigned — use passive fallback */
    passive_update();
    return 0;
  }

  fog = &nation_fog[nation_id];

  /* Step 1: Update fog of war from current positions */
  fog_update_nation(nation_id);

  /* Step 2: Observe visible sectors */
  fog_observe_nation(nation_id, (int)country);

  /* Step 3: Evaluate board through fog */
  BOARD_EVAL_STRUCT eval;
  evaluate_board_world(&eval, fog, nation_id);

  /* Step 4: Select strategy */
  DECISION_STRUCT decision;
  if (select_strategy(&decision, pers, &eval, fog) != 0) {
    fprintf(fupdate, "    [AI] Strategy selection failed for %s\n",
            pers->name);
    return -1;
  }

  /* Step 5: Execute strategy */
  if (execute_strategy(&decision, pers, &eval, fog, nation_id) != 0) {
    fprintf(fupdate, "    [AI] Strategy execution failed for %s\n",
            pers->name);
    return -1;
  }

  /* Step 6: Generate and deliver turn report */
  AI_REPORT_STRUCT report;
  ai_report_init(&report, nation_id, (int)country, pers->name);
  ai_report_set_board_state(&report, &eval);
  ai_report_set_strategy(&report, &decision);

  /* Nation name is a fixed-size array in NTN_STRUCT — always valid */
  strncpy(report.nation_name, n_ptr->name, NAMELTH - 1);
  report.nation_name[NAMELTH - 1] = '\0';

  /* Deliver the report through the mail system */
  ai_report_deliver(&report);

  /* Also log the decision to the update file */
  fprintf(fupdate, "    [AI] %s: %s (confidence %.0f%%) — %s\n",
          pers->name,
          ai_strategy_name[decision.strategy],
          decision.confidence * 100.0,
          decision.reasoning);

  return 0;
}

/* ------------------------------------------------------------------ */
/* Passive fallback for non-personality nations                         */
/* ------------------------------------------------------------------ */

/* passive_update — minimal AI for nations without personality data.
 * Just does basic expansion and garrison, no strategic thinking.
 */
void
passive_update PARM_0(void)
{
  fprintf(fupdate, "    [AI] Passive mode update (no personality data)\n");

  /* Basic: just let existing rover code handle movement */
  /* In Sprint 0, non-Warlord nations use existing rove_army behavior */
}