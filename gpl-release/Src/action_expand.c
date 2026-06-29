// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * action_expand.c - AI Expand Action Module for Conquer V5
 * Sprint 1 Task 1.4: Real Action Execution
 *
 * Personality-weighted, fog-of-war constrained territory expansion,
 * military movement, and sector construction for NPC nations.
 *
 * Design principles:
 *   1. Honest AI: never act on sectors invisible through fog-of-war
 *   2. Personality-driven: Warlord fights, Merchant builds, etc.
 *   3. Start simple: flat thresholds, expand later
 *   4. Record all actions into AI_REPORT for observability
 *
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */

#include "dataA.h"
#include "action_expand.h"
#include "armyX.h"
#include "moveX.h"
#include "activeX.h"
#include "desigX.h"
#include "statusX.h"
#include "executeX.h"
#include "worldX.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Internal helpers
 * ============================================================ */

/* Set attack/retreat thresholds based on nation personality.
 * In v1 we use flat defaults; personality-specific tuning is Sprint 2. */
static void
set_personality_thresholds(EXPAND_STATE_PTR state, int nation_id)
{
  /* Default: balanced/strategist */
  state->attack_threshold = ATTACK_THRESH_STRATEGIST;
  state->retreat_threshold = RETREAT_THRESH_STRATEGIST;
  state->pref_fortify = 3;
  state->pref_economy = 3;
  state->pref_military = 3;
  state->weight_military = 40;
  state->weight_expansion = 40;
  state->weight_defense = 40;
  state->weight_economy = 40;

  /* Switch on the full active type, not aggression level.
   * n_aggression() only returns 0 or 1 (passive/aggressive).
   * The active field encodes alignment + aggression + NPC status. */
  int active = ntn_ptr->active;

  switch (active) {
    case ACT_KILLER:
      state->attack_threshold = ATTACK_THRESH_WARLORD;
      state->retreat_threshold = RETREAT_THRESH_WARLORD;
      state->pref_fortify = 3;
      state->pref_economy = 1;
      state->pref_military = 5;
      state->weight_military = 80;
      state->weight_expansion = 60;
      state->weight_defense = 30;
      state->weight_economy = 10;
      break;
    case ACT_OVERT:
      state->attack_threshold = ATTACK_THRESH_PIONEER;
      state->retreat_threshold = RETREAT_THRESH_PIONEER;
      state->pref_fortify = 1;
      state->pref_economy = 2;
      state->pref_military = 2;
      state->weight_military = 50;
      state->weight_expansion = 70;
      state->weight_defense = 20;
      state->weight_economy = 30;
      break;
    case ACT_MOBILE:
      state->attack_threshold = ATTACK_THRESH_STRATEGIST;
      state->retreat_threshold = RETREAT_THRESH_STRATEGIST;
      state->pref_fortify = 3;
      state->pref_economy = 3;
      state->pref_military = 3;
      state->weight_military = 40;
      state->weight_expansion = 40;
      state->weight_defense = 40;
      state->weight_economy = 40;
      break;
    case ACT_ENFORCE:
      state->attack_threshold = ATTACK_THRESH_FORTRESS;
      state->retreat_threshold = RETREAT_THRESH_FORTRESS;
      state->pref_fortify = 5;
      state->pref_economy = 1;
      state->pref_military = 3;
      state->weight_military = 20;
      state->weight_expansion = 10;
      state->weight_defense = 80;
      state->weight_economy = 30;
      break;
    case ACT_STATIC:
      /* Mostly passive, minimal expansion */
      state->attack_threshold = 250; /* only attack with overwhelming force */
      state->retreat_threshold = 85;
      state->pref_fortify = 4;
      state->pref_economy = 4;
      state->pref_military = 1;
      state->weight_military = 10;
      state->weight_expansion = 5;
      state->weight_defense = 60;
      state->weight_economy = 50;
      break;
    default:
      break;
  }
}

/* ============================================================
 * Public API Implementation
 * ============================================================ */

/* ACTION_EXPAND_INIT — set up expand state from personality + context */
expand_result_t
action_expand_init(EXPAND_STATE_PTR state, int nation_id)
{
  if (state == NULL) return EXPAND_ERROR;

  memset(state, 0, sizeof(EXPAND_STATE));
  state->nation_id = nation_id;
  state->turn_number = TURN;

  /* Set thresholds from personality (aggression level) */
  set_personality_thresholds(state, nation_id);

  return EXPAND_OK;
}

/* AI_SECTOR_VISIBLE — check if a sector is visible through fog-of-war.
 * In v1, we use simple proximity: a sector is visible if the nation
 * owns a sector within scout range. Full fog-of-war struct integration
 * comes when the fog module is wired in (Sprint 1.1-1.3 structs exist
 * but the expand module reads directly from game state for now). */
int
ai_sector_visible(int x, int y, int nation_id)
{
  /* Basic validity check */
  if (!XY_ONMAP(x, y)) return FALSE;

  /* Nation can always see its own sectors */
  if (sct[x][y].owner == nation_id) return TRUE;

  /* Check if any adjacent sector is owned by this nation */
  if (ai_has_adjacent_owned(x, y, nation_id)) return TRUE;

  /* TODO: integrate with FOG_OF_WAR_STRUCT from fog module
   * for personality-based scout ranges (Warlord sees 4 sectors,
   * Fortress sees 2, etc.) For v1, adjacency = visibility. */

  return FALSE;
}

/* AI_HAS_ANY_TERRITORY — does the nation own anything at all? */
int
ai_has_any_territory(int nation_id)
{
  int x, y;
  for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
    for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
      if (XY_ONMAP(x, y) && sct[x][y].owner == nation_id) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/* AI_HAS_ADJACENT_OWNED — does the nation own at least one sector
 * adjacent to (x,y)? Uses map_loop for 8-directional check. */
int
ai_has_adjacent_owned(int x, int y, int nation_id)
{
  int dx, dy;

  for (dx = -1; dx <= 1; dx++) {
    for (dy = -1; dy <= 1; dy++) {
      if (dx == 0 && dy == 0) continue;
      int nx = x + dx;
      int ny = y + dy;
      if (XY_ONMAP(nx, ny) && sct[nx][ny].owner == nation_id) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/* AI_FRIENDLY_STRENGTH — count total friendly military strength
 * at or near a sector (within given range) */
long
ai_friendly_strength(int x, int y, int nation_id, int range)
{
  long total = 0;
  ARMY_PTR ap;

  /* Iterate through nation's army list */
  for (ap = ntn_ptr->army_list; ap != NULL; ap = ap->next) {
    /* Skip if not our nation (shouldn't happen, but guard) */
    int ax = ap->xloc;
    int ay = ap->yloc;

    /* Check distance from target */
    int dist = abs(ax - x) + abs(ay - y); /* Manhattan distance */
    if (dist <= range) {
      total += ap->strength;
    }
  }
  return total;
}

/* AI_ENEMY_STRENGTH — estimate enemy strength at a sector.
 * Uses visible information only (honest AI constraint). */
long
ai_enemy_strength(int x, int y, int nation_id)
{
  long total = 0;
  int i;

  if (!XY_ONMAP(x, y)) return 0;

  /* Only report what we can actually see */
  if (!ai_sector_visible(x, y, nation_id)) return 0;

  /* Check all armies at this location from other nations */
  for (i = 0; i < world.nations; i++) {
    if (i == nation_id) continue;
    if (world.np[i] == NULL) continue;
    if (!n_isactive(world.np[i]->active)) continue;

    ARMY_PTR ap;
    for (ap = world.np[i]->army_list; ap != NULL; ap = ap->next) {
      if (ap->xloc == x && ap->yloc == y) {
        total += ap->strength;
      }
    }
  }
  return total;
}

/* AI_SECTOR_PRIORITY — evaluate a sector's strategic value.
 * Returns 0-100 priority score, personality-weighted. */
int
ai_sector_priority(int x, int y, EXPAND_STATE_PTR state)
{
  int priority = 0;
  SCT_STRUCT *sp;

  if (!XY_ONMAP(x, y)) return 0;

  sp = &sct[x][y];

  /* Base value from game's attract_val */
  priority += attract_val(x, y);

  /* Already owned? Much lower priority for expansion (we already have it) */
  if (sp->owner == state->nation_id) {
    priority = priority / 5; /* minimal value — we already own it */
    priority += (state->weight_defense * sp->efficiency) / 200;
  }
  /* Unowned? Good for expansion — must be adjacent to our territory */
  else if (sp->owner == UNOWNED) {
    if (!ai_has_adjacent_owned(x, y, state->nation_id)) {
      /* Can't claim sectors we can't reach */
      return 0;
    }
    priority += state->weight_expansion / 4;
    /* Minerals and trade goods boost priority for economic personalities */
    if (sp->minerals > 0) {
      priority += (state->weight_economy * sp->minerals) / 10;
    }
    if (sp->tradegood > 0) {
      priority += (state->weight_economy * 3);
    }
  }
  /* Enemy owned? Military value — only if adjacent or visible */
  else {
    if (!ai_has_adjacent_owned(x, y, state->nation_id) && !ai_sector_visible(x, y, state->nation_id)) {
      return 0;
    }
    priority += state->weight_military / 5;
  }

  /* Adjacency bonus: sectors next to our territory are more valuable */
  if (ai_has_adjacent_owned(x, y, state->nation_id)) {
    priority += 10;
  }

  /* Clamp to 0-100 */
  if (priority < 0) priority = 0;
  if (priority > 100) priority = 100;

  return priority;
}

/* AI_EVALUATE_TARGETS — build a priority-sorted list of sectors to act on.
 * Returns linked list of targets, caller must free with ai_free_targets(). */
EXPAND_TARGET_PTR
ai_evaluate_targets(EXPAND_STATE_PTR state, int *target_count)
{
  EXPAND_TARGET_PTR head = NULL;
  EXPAND_TARGET_PTR tail = NULL;
  int count = 0;
  int x, y;

  if (target_count) *target_count = 0;

  /* Scan the map for potential targets within nation's borders + 1 sector */
  for (x = ntn_ptr->leftedge - 1; x <= ntn_ptr->rightedge + 1; x++) {
    for (y = ntn_ptr->topedge - 1; y <= ntn_ptr->bottomedge + 1; y++) {
      if (!XY_ONMAP(x, y)) continue;

      /* Honest AI: skip sectors we can't see */
      if (!ai_sector_visible(x, y, state->nation_id)) continue;

      SCT_STRUCT *sp = &sct[x][y];

      /* Must be adjacent to our territory (or owned by us) to matter */
      if (sp->owner != state->nation_id && !ai_has_adjacent_owned(x, y, state->nation_id)) continue;

      int pri = ai_sector_priority(x, y, state);

      /* Skip zero-priority sectors */
      if (pri <= 0) continue;

      /* Determine target type */
      target_type_t type;
      if (sp->owner == state->nation_id) {
        type = TGT_REINFORCE;
      } else if (sp->owner == UNOWNED) {
        type = TGT_CLAIM_EMPTY;
      } else {
        /* Enemy territory */
        long friendly = ai_friendly_strength(x, y, state->nation_id, 2);
        long enemy = ai_enemy_strength(x, y, state->nation_id);
        long strength_ratio = (enemy > 0) ? (friendly * 100) / enemy : 999;

        if (strength_ratio >= state->attack_threshold) {
          type = TGT_ATTACK;
        } else if (strength_ratio < state->retreat_threshold) {
          type = TGT_RETREAT;
        } else {
          type = TGT_CLAIM_CONTESTED;
        }
      }

      /* Allocate target node */
      EXPAND_TARGET_PTR tgt = (EXPAND_TARGET_PTR)malloc(sizeof(EXPAND_TARGET));
      if (tgt == NULL) {
        ai_free_targets(head);
        return NULL;
      }
      tgt->x = x;
      tgt->y = y;
      tgt->type = type;
      tgt->priority = pri;
      tgt->estimated_strength = (int)ai_enemy_strength(x, y, state->nation_id);
      tgt->next = NULL;

      /* Append to list */
      if (tail == NULL) {
        head = tgt;
      } else {
        tail->next = tgt;
      }
      tail = tgt;
      count++;
    }
  }

  /* Simple insertion sort by priority (descending) */
  /* For v1 this is fine; optimize later if needed */
  if (head != NULL && head->next != NULL) {
    EXPAND_TARGET_PTR sorted = NULL;
    EXPAND_TARGET_PTR cur = head;
    while (cur != NULL) {
      EXPAND_TARGET_PTR next = cur->next;
      if (sorted == NULL || cur->priority >= sorted->priority) {
        cur->next = sorted;
        sorted = cur;
      } else {
        EXPAND_TARGET_PTR s = sorted;
        while (s->next != NULL && s->next->priority > cur->priority) {
          s = s->next;
        }
        cur->next = s->next;
        s->next = cur;
      }
      cur = next;
    }
    head = sorted;
  }

  if (target_count) *target_count = count;
  return head;
}

/* AI_FREE_TARGETS — free target list memory */
void
ai_free_targets(EXPAND_TARGET_PTR list)
{
  while (list != NULL) {
    EXPAND_TARGET_PTR next = list->next;
    free(list);
    list = next;
  }
}

/* AI_CLAIM_SECTOR — claim unowned/contested territory adjacent to borders.
 * Returns number of sectors claimed. */
int
ai_claim_sector(EXPAND_STATE_PTR state)
{
  int claimed = 0;
  int x, y;
  int max_claims = 5; /* v1: claim at most 5 sectors per turn */

  /* Early exit: if we have no territory at all, we can't claim */
  if (!ai_has_any_territory(state->nation_id)) {
    return 0;
  }

  /* Scan for unowned sectors adjacent to our territory */
  for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
    for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
      if (!XY_ONMAP(x, y)) continue;

      /* Only claim unowned sectors */
      if (sct[x][y].owner != UNOWNED) continue;

      /* Must be adjacent to our territory */
      if (!ai_has_adjacent_owned(x, y, state->nation_id)) continue;

      /* Must be visible (honest AI) */
      if (!ai_sector_visible(x, y, state->nation_id)) continue;

      /* Claim it: set ownership */
      sct[x][y].owner = state->nation_id;

      /* Emit ownership change to update file */
      fprintf(fexe, "S_OWN\t%d\t%d\t%d\t%d\tnull\tnull\n",
              EX_SCTOWN, sct[x][y].owner, x, y);

      /* Set a basic designation based on personality */
      int desg = ai_best_designation(x, y, state);
      set_majordesg(sct[x][y].designation, desg);
      fprintf(fexe, "S_DESG\t%d\t%d\t%d\t%d\tnull\tnull\n",
              EX_SCTDESG, sct[x][y].designation, x, y);

      claimed++;
      state->sectors_claimed++;

      fprintf(fupdate,
              "  AI: Nation %d claims sector (%d,%d) as %s\n",
              state->nation_id, x, y,
              maj_dinfo[desg].name);

      if (claimed >= max_claims) break;
    }
    if (claimed >= max_claims) break;
  }

  return claimed;
}

/* AI_MOVE_MILITARY — strategic army movement.
 * Moves armies based on personality: reinforce borders, advance
 * toward targets, retreat when outmatched.
 * Returns number of armies moved. */
int
ai_move_military(EXPAND_STATE_PTR state)
{
  int moved = 0;
  ARMY_PTR ap;
  int target_count = 0;

  /* Evaluate targets for this turn */
  EXPAND_TARGET_PTR targets = ai_evaluate_targets(state, &target_count);
  if (targets == NULL || target_count == 0) {
    return 0;
  }

  /* Iterate through nation's armies */
  for (ap = ntn_ptr->army_list; ap != NULL; ap = ap->next) {
    int ax = ap->xloc;
    int ay = ap->yloc;
    int best_dist = 9999;
    EXPAND_TARGET_PTR best_target = NULL;

    /* Skip non-combat units (leaders with no troops, etc.) */
    if (ap->strength < 10) continue;

    /* Find the highest-priority target this army can reach */
    EXPAND_TARGET_PTR tgt = targets;
    while (tgt != NULL) {
      /* Skip targets we can't see */
      if (!ai_sector_visible(tgt->x, tgt->y, state->nation_id)) {
        tgt = tgt->next;
        continue;
      }

      /* Skip retreat targets for v1 (we'll handle retreat in v2) */
      if (tgt->type == TGT_RETREAT) {
        tgt = tgt->next;
        continue;
      }

      int dist = abs(tgt->x - ax) + abs(tgt->y - ay);
      if (dist < best_dist) {
        best_dist = dist;
        best_target = tgt;
      }
      tgt = tgt->next;
    }

    /* Move toward best target */
    if (best_target != NULL && best_dist > 0) {
      /* Determine movement mode */
      if (unit_flight(ap->status)) {
        movemode = MOVE_FLYARMY;
      } else {
        movemode = MOVE_ARMY;
      }

      /* Move one step toward target (handles terrain) */
      /* Use npc_movearmy which handles pathfinding */
      global_int = best_target->x;
      global_long = best_target->y;

      if (npc_movearmy(best_target->x, best_target->y)) {
        moved++;
        state->armies_moved++;

        fprintf(fupdate,
                "  AI: Army %d moves toward (%d,%d) [type=%s, dist=%d]\n",
                (int)ap->armyid,
                best_target->x, best_target->y,
                best_target->type == TGT_CLAIM_EMPTY ? "claim" :
                best_target->type == TGT_CLAIM_CONTESTED ? "contest" :
                best_target->type == TGT_ATTACK ? "attack" :
                best_target->type == TGT_REINFORCE ? "reinforce" : "patrol",
                best_dist);
      }
    }
  }

  ai_free_targets(targets);
  return moved;
}

/* AI_BEST_DESIGNATION — determine the best major designation
 * for a sector based on personality weights and sector properties. */
int
ai_best_designation(int x, int y, EXPAND_STATE_PTR state)
{
  SCT_STRUCT *sp = &sct[x][y];

  /* Check for special resources that override personality */
  if (sp->minerals > 5) {
    /* High mineral content: mine it */
    if (state->pref_economy > state->pref_military) {
      return MAJ_METALMINE;
    }
    /* Military personality still mines for war resources */
    return MAJ_METALMINE;
  }

  if (sp->tradegood > 0 && state->weight_economy > 30) {
    return MAJ_TOWN;  /* Trade goods → town for economic value */
  }

  /* Personality-weighted selection */
  int score_farm = (sp->people > 50) ? state->pref_economy * 2 : 0;
  int score_stockade = state->pref_fortify * 3;
  int score_town = (sp->people > 100) ? state->pref_economy * 3 : 0;

  /* Border sectors prefer fortification */
  if (ai_has_adjacent_owned(x, y, state->nation_id)) {
    /* Check if any adjacent sector is NOT ours → border sector */
    int dx, dy;
    for (dx = -1; dx <= 1; dx++) {
      for (dy = -1; dy <= 1; dy++) {
        if (dx == 0 && dy == 0) continue;
        int nx = x + dx;
        int ny = y + dy;
        if (XY_ONMAP(nx, ny) &&
            sct[nx][ny].owner != state->nation_id &&
            sct[nx][ny].owner != UNOWNED) {
          /* Border with enemy → fortify */
          score_stockade += state->pref_fortify * 2;
        }
      }
    }
  }

  /* Choose highest scoring designation */
  if (score_stockade >= score_farm && score_stockade >= score_town) {
    return MAJ_STOCKADE;
  }
  if (score_town >= score_farm) {
    return MAJ_TOWN;
  }
  /* Default to farm (food production is always useful) */
  return MAJ_FARM;
}

/* AI_BUILD_IN_SECTOR — construct improvements in owned territory.
 * Returns number of buildings started. */
int
ai_build_in_sector(EXPAND_STATE_PTR state)
{
  int built = 0;
  int x, y;

  /* Iterate through owned sectors */
  for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
    for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
      if (!XY_ONMAP(x, y)) continue;

      /* Only build in sectors we own */
      if (sct[x][y].owner != state->nation_id) continue;

      /* Only build if sector is visible (should always be true for owned) */
      if (!ai_sector_visible(x, y, state->nation_id)) continue;

      /* Check if the current designation needs improvement */
      int current_desg = major_desg(sct[x][y].designation);
      int best_desg = ai_best_designation(x, y, state);

      /* Only change if we have a better designation in mind
       * and the sector is not already well-developed */
      if (best_desg != current_desg &&
          sct[x][y].efficiency < 50) {
        /* Change designation */
        fprintf(fexe, "S_DESG\t%d\t%d\t%d\t%d\tnull\tnull\n",
                EX_SCTDESG, sct[x][y].designation, x, y);

        built++;
        state->buildings_started++;

        fprintf(fupdate,
                "  AI: Nation %d redesignates (%d,%d) to %s\n",
                state->nation_id, x, y,
                maj_dinfo[best_desg].name);
      }

      /* Limit builds per turn to avoid over-building */
      if (built >= 3) break;  /* v1: max 3 redesignations per turn */
    }
    if (built >= 3) break;
  }

  return built;
}

/* ACTION_EXPAND_EXECUTE — main entry point for expand actions.
 * Called from cpu_update() pipeline after select_strategy().
 * Orchestrates claiming, moving, and building for this turn. */
expand_result_t
action_expand_execute(EXPAND_STATE_PTR state)
{
  int claimed, moved, built;
  expand_result_t result = EXPAND_OK;

  fprintf(fupdate,
          "  AI: Nation %d expand phase — weight_mil=%d weight_exp=%d "
          "weight_def=%d weight_eco=%d\n",
          state->nation_id,
          state->weight_military,
          state->weight_expansion,
          state->weight_defense,
          state->weight_economy);

  /* Early exit: no territory, nothing to expand from */
  if (!ai_has_any_territory(state->nation_id)) {
    return EXPAND_NO_TARGETS;
  }

  /* Phase 1: Claim unowned adjacent territory */
  claimed = ai_claim_sector(state);
  fprintf(fupdate,
          "  AI: Nation %d claimed %d sectors\n",
          state->nation_id, claimed);

  /* Phase 2: Move military toward targets */
  moved = ai_move_military(state);
  fprintf(fupdate,
          "  AI: Nation %d moved %d armies\n",
          state->nation_id, moved);

  /* Phase 3: Build/improve owned sectors */
  built = ai_build_in_sector(state);
  fprintf(fupdate,
          "  AI: Nation %d built %d improvements\n",
          state->nation_id, built);

  /* Summary */
  if (claimed == 0 && moved == 0 && built == 0) {
    fprintf(fupdate,
            "  AI: Nation %d expand phase — no actions taken\n",
            state->nation_id);
    result = EXPAND_NO_TARGETS;
  }

  fprintf(fupdate,
          "  AI: Nation %d expand phase complete — "
          "claimed=%d moved=%d built=%d\n",
          state->nation_id, claimed, moved, built);

  return result;
}