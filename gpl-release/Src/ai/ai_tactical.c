/* ai_tactical.c — Tactical AI Module for Conquer V5
 * Sprint 2 Task 2.2: Combat engagement, retreat, garrison, reinforcement
 *
 * This module handles turn-level combat decisions for NPC nations.
 * All decisions are personality-weighted and fog-of-war constrained.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */
#ifdef MEMORYH
#include "dataA.h"
#else
#include "ai/ai_standalone_types.h"
#endif
#include "ai/ai_tactical.h"
#include "ai/personality.h"
#ifdef MEMORYH
#include "ai/ai_integration.h"
#endif
#include "ai/fog_of_war.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Personality → Threshold Mapping
 *
 * In game mode: reads from loaded PERSONALITY_STRUCT (JSON).
 * In standalone mode: falls back to hardcoded defaults.
 * Difficulty multipliers are applied AFTER personality values.
 * ============================================================ */

static void
set_attack_thresholds(TACTICAL_STATE_PTR state, int personality_type)
{
    /* Default hardcoded values — used when no personality registry available */
    switch (personality_type) {
    case ACT_OVERT:   /* Warlord */
        state->attack_ratio = TAC_ATTACK_WARLORD;
        state->retreat_ratio = TAC_RETREAT_WARLORD;
        state->garrison_threshold = TAC_GARRISON_WARLORD;
        state->aggression = 80;
        state->caution = 20;
        state->border_focus = 30;
        break;
    case ACT_MOBILE:  /* Pioneer */
        state->attack_ratio = TAC_ATTACK_PIONEER;
        state->retreat_ratio = TAC_RETREAT_PIONEER;
        state->garrison_threshold = TAC_GARRISON_PIONEER;
        state->aggression = 60;
        state->caution = 40;
        state->border_focus = 50;
        break;
    case ACT_KILLER:  /* Strategist */
        state->attack_ratio = TAC_ATTACK_STRATEGIST;
        state->retreat_ratio = TAC_RETREAT_STRATEGIST;
        state->garrison_threshold = TAC_GARRISON_STRATEGIST;
        state->aggression = 70;
        state->caution = 30;
        state->border_focus = 40;
        break;
    case ACT_GUERRILA: /* Merchant */
        state->attack_ratio = TAC_ATTACK_MERCHANT;
        state->retreat_ratio = TAC_RETREAT_MERCHANT;
        state->garrison_threshold = TAC_GARRISON_MERCHANT;
        state->aggression = 30;
        state->caution = 60;
        state->border_focus = 40;
        break;
    case ACT_ENFORCE: /* Fortress */
        state->attack_ratio = TAC_ATTACK_FORTRESS;
        state->retreat_ratio = TAC_RETREAT_FORTRESS;
        state->garrison_threshold = TAC_GARRISON_FORTRESS;
        state->aggression = 20;
        state->caution = 70;
        state->border_focus = 80;
        break;
    default:
        state->attack_ratio = 130;
        state->retreat_ratio = 50;
        state->garrison_threshold = 50;
        state->aggression = 50;
        state->caution = 50;
        state->border_focus = 50;
        break;
    }
}

/* ============================================================
 * AI_TACTICAL_INIT — Initialize tactical state from personality
 *
 * In game mode: reads from PERSONALITY_REGISTRY (JSON-driven).
 * Falls back to hardcoded thresholds if no registry available.
 * Applies difficulty multipliers after personality values.
 * ============================================================ */

int
ai_tactical_init(TACTICAL_STATE_PTR state, int nation_id)
{
  PERSONALITY_REGISTRY_PTR registry = NULL;
  PERSONALITY_PTR pers = NULL;
  const DIFFICULTY_CONFIG *diff = NULL;

  if (state == NULL) return -1;

  memset(state, 0, sizeof(TACTICAL_STATE));
  state->nation_id = nation_id;

#ifdef MEMORYH
  /* Try to load from personality registry (JSON-driven) */
  registry = ai_get_registry();
  if (registry != NULL) {
    pers = personality_for_nation(registry, nation_id);
  }

  if (pers != NULL && pers->loaded) {
    /* JSON-driven values: convert 0.0-1.0 ranges to integer thresholds */
    /* attack_preference (0.0-1.0) → attack_ratio (100-200)
     *   0.0 = never attack → ratio 200 (need 2:1)
     *   1.0 = always attack → ratio 100 (parity)
     */
    state->attack_ratio = (int)(200.0 - (pers->attack_preference * 100.0));

    /* retreat_threshold (0.0-1.0) → retreat_ratio (20-80)
     *   0.0 = never retreat → ratio 20
     *   1.0 = retreat easily → ratio 80
     */
    state->retreat_ratio = (int)(20.0 + (pers->retreat_threshold * 60.0));

    /* territory_focus (0.0-1.0) → garrison_threshold (20-80)
     *   0.0 = minimal garrison → threshold 80
     *   1.0 = garrison everything → threshold 20
     */
    state->garrison_threshold = (int)(80.0 - (pers->territory_focus * 60.0));

    /* aggression/caution/border_focus from priority weights */
    state->aggression = (int)(pers->attack_preference * 100.0);
    state->caution = (int)((1.0 - pers->attack_preference) * 100.0);
    state->border_focus = (int)(pers->base_priority.defense * 100.0);

    fprintf(fupdate,
            "  TAC: Nation %d loaded from personality '%s' "
            "(attack_pref=%.2f, retreat=%.2f, territory=%.2f)\n",
            nation_id, pers->name,
            pers->attack_preference,
            pers->retreat_threshold,
            pers->territory_focus);
  } else
#endif
  {
    /* Fallback: hardcoded thresholds from personality type */
    int personality_type = ACT_OVERT;
#ifdef MEMORYH
    if (ntn_ptr != NULL && ntn_ptr->active > 0) {
      personality_type = ntn_ptr->active;
    }
#endif
    set_attack_thresholds(state, personality_type);
  }

  /* Apply difficulty multipliers */
  if (registry != NULL) {
    diff = personality_get_difficulty(registry);
    if (diff != NULL) {
      /* Higher difficulty → lower attack_ratio = AI attacks more aggressively */
      state->attack_ratio = (int)((double)state->attack_ratio / diff->attack_mult);
      /* Higher difficulty → lower retreat_ratio = AI retreats less */
      state->retreat_ratio = (int)((double)state->retreat_ratio / diff->attack_mult);
    }
  }

  return 0;
}

/* ============================================================
 * AI_TACTICAL_EVALUATE — Build target list for tactical decisions
 * ============================================================ */

TACTICAL_TARGET_PTR
ai_tactical_evaluate(TACTICAL_STATE_PTR state, int *target_count)
{
    TACTICAL_TARGET_PTR head = NULL, tail = NULL;
    int count = 0;
    int x, y;

    if (state == NULL || target_count == NULL) return NULL;
    *target_count = 0;

#ifdef MEMORYH
    /* Iterate through visible sectors near our territory */
    for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
        for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
            if (!XY_ONMAP(x, y)) continue;

            SCT_STRUCT *sp = &sct[x][y];

            /* Must be visible through fog */
            if (!ai_sector_visible(x, y, state->nation_id)) continue;

            /* Skip sectors we own — garrisons handle those */
            if (sp->owner == state->nation_id) continue;

            /* Determine target type based on ownership and strength */
            tactical_type_t type;
            int priority = 0;
            int est_enemy = 0;
            int est_friendly = 0;

            if (sp->owner == UNOWNED) {
                /* Unowned: low priority scouting target */
                type = TAC_SCOUT;
                priority = 10;
            } else {
                /* Enemy territory */
                est_enemy = (int)ai_enemy_strength(x, y, state->nation_id);
                est_friendly = (int)ai_friendly_strength(x, y, state->nation_id, 2);

                /* Strength ratio: higher = we're stronger */
                int ratio = (est_enemy > 0)
                    ? (est_friendly * 100) / est_enemy
                    : 999;

                if (ratio >= state->attack_ratio) {
                    /* Strong enough to attack */
                    type = TAC_ATTACK;
                    priority = 50 + (ratio / 5);
                    if (priority > 95) priority = 95;
                } else if (ratio < state->retreat_ratio) {
                    /* Outmatched: consider retreat from nearby sectors */
                    type = TAC_RETREAT;
                    priority = 70; /* High priority to save troops */
                } else {
                    /* Contested: hold position or reinforce */
                    type = TAC_REINFORCE;
                    priority = 30 + (state->border_focus / 3);
                }
            }

            /* Skip zero-priority targets */
            if (priority <= 0) continue;

            /* Allocate target */
            TACTICAL_TARGET_PTR tgt = (TACTICAL_TARGET_PTR)malloc(sizeof(TACTICAL_TARGET));
            if (tgt == NULL) {
                ai_tactical_free_targets(head);
                return NULL;
            }
            tgt->x = x;
            tgt->y = y;
            tgt->type = type;
            tgt->priority = priority;
            tgt->estimated_enemy = est_enemy;
            tgt->estimated_friendly = est_friendly;
            tgt->confidence = 80; /* Will be adjusted by fog freshness later */
            tgt->next = NULL;

            if (tail == NULL) {
                head = tgt;
            } else {
                tail->next = tgt;
            }
            tail = tgt;
            count++;
        }
    }
#endif

    *target_count = count;
    return head;
}

/* ============================================================
 * AI_SHOULD_ATTACK — Decide if we should attack a position
 * ============================================================ */

int
ai_should_attack(int x, int y, TACTICAL_STATE_PTR state)
{
    if (state == NULL) return 0;

#ifdef MEMORYH
    if (!XY_ONMAP(x, y)) return 0;

    /* Can't attack what we can't see */
    if (!ai_sector_visible(x, y, state->nation_id)) return 0;

    /* Can't attack our own sectors */
    if (sct[x][y].owner == state->nation_id) return 0;

    long friendly = ai_friendly_strength(x, y, state->nation_id, 2);
    long enemy = ai_enemy_strength(x, y, state->nation_id);

    /* No enemy = no attack needed (claim instead) */
    if (enemy <= 0) return 0;

    /* Check strength ratio against personality threshold */
    int ratio = (int)((friendly * 100) / enemy);

    return (ratio >= state->attack_ratio) ? 1 : 0;
#else
    /* Standalone mode: basic check */
    return (state->aggression >= 50);
#endif
}

/* ============================================================
 * AI_SHOULD_RETREAT — Decide if we should pull back
 * ============================================================ */

int
ai_should_retreat(int x, int y, TACTICAL_STATE_PTR state)
{
    if (state == NULL) return 0;

#ifdef MEMORYH
    if (!XY_ONMAP(x, y)) return 0;

    long friendly = ai_friendly_strength(x, y, state->nation_id, 1);
    long enemy = ai_enemy_strength(x, y, state->nation_id);

    /* No threat = no retreat */
    if (enemy <= 0) return 0;

    int ratio = (int)((friendly * 100) / enemy);

    /* Retreat if we're outmatched beyond our comfort level */
    return (ratio < state->retreat_ratio) ? 1 : 0;
#else
    /* Standalone mode: based on caution */
    return (state->caution >= 50);
#endif
}

/* ============================================================
 * AI_PLACE_GARRISONS — Assign armies to defend border sectors
 * ============================================================ */

int
ai_place_garrisons(TACTICAL_STATE_PTR state)
{
    int garrisons = 0;
    int x, y;

    if (state == NULL) return 0;

#ifdef MEMORYH
    /* Find border sectors (owned by us, adjacent to non-us) */
    for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
        for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
            if (!XY_ONMAP(x, y)) continue;
            if (sct[x][y].owner != state->nation_id) continue;

            /* Check if this is a border sector */
            int is_border = 0;
            int threat_level = 0;
            int dx, dy;

            for (dx = -1; dx <= 1; dx++) {
                for (dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx;
                    int ny = y + dy;
                    if (!XY_ONMAP(nx, ny)) continue;

                    if (sct[nx][ny].owner != state->nation_id &&
                        sct[nx][ny].owner != UNOWNED) {
                        is_border = 1;
                        /* Estimate threat from this neighbor */
                        threat_level += (int)ai_enemy_strength(nx, ny, state->nation_id) / 10;
                    }
                }
            }

            /* Only garrison if border and threat exceeds threshold */
            if (!is_border) continue;
            if (threat_level < state->garrison_threshold) continue;

            /* Find an available army to garrison here */
            for (army_ptr = ntn_ptr->army_list;
                 army_ptr != NULL;
                 army_ptr = army_ptr->next) {
                /* Skip armies already on garrison duty */
                if (ARMY_STAT == ST_GARRISON) continue;
                /* Skip armies already at this location */
                if (ARMY_XLOC == x && ARMY_YLOC == y) {
                    set_status(ARMY_STAT, ST_GARRISON);
                    garrisons++;
                    break; /* One garrison per sector */
                }
            }
        }
    }
#endif

    if (state != NULL) state->garrisons_placed += garrisons;
    return garrisons;
}

/* ============================================================
 * AI_REINFORCE_BORDERS — Move reserves to threatened sectors
 * ============================================================ */

int
ai_reinforce_borders(TACTICAL_STATE_PTR state)
{
    int reinforced = 0;
    int x, y;

    if (state == NULL) return 0;

#ifdef MEMORYH
    /* Find our most threatened border sectors */
    int worst_x = -1, worst_y = -1;
    int worst_threat = 0;

    for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
        for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
            if (!XY_ONMAP(x, y)) continue;
            if (sct[x][y].owner != state->nation_id) continue;

            /* Calculate threat for this owned sector */
            int threat = 0;
            int dx, dy;
            for (dx = -1; dx <= 1; dx++) {
                for (dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx;
                    int ny = y + dy;
                    if (!XY_ONMAP(nx, ny)) continue;
                    if (sct[nx][ny].owner != state->nation_id &&
                        sct[nx][ny].owner != UNOWNED) {
                        threat += (int)ai_enemy_strength(nx, ny, state->nation_id);
                    }
                }
            }

            if (threat > worst_threat) {
                worst_threat = threat;
                worst_x = x;
                worst_y = y;
            }
        }
    }

    /* If we found a threatened sector, move reserves toward it */
    if (worst_x >= 0 && worst_y >= 0) {
        for (army_ptr = ntn_ptr->army_list;
             army_ptr != NULL;
             army_ptr = army_ptr->next) {
            /* Skip armies already in combat or garrison */
            if (ARMY_STAT == ST_GARRISON) continue;
            if (ARMY_STAT == ST_ATTACK) continue;

            /* Skip armies at the threatened sector already */
            if (ARMY_XLOC == worst_x && ARMY_YLOC == worst_y) continue;

            /* Move the first available reserve */
            if (unit_flight(ARMY_TYPE)) {
                movemode = MOVE_FLYARMY;
            } else {
                movemode = MOVE_ARMY;
            }

            if (npc_movearmy(worst_x, worst_y)) {
                reinforced++;
                fprintf(fupdate,
                        "  TAC: Nation %d reinforces (%d,%d) threat=%d\n",
                        state->nation_id, worst_x, worst_y, worst_threat);
                break; /* Move one reserve per call */
            }
        }
    }
#endif

    if (state != NULL) state->reinforcements_sent += reinforced;
    return reinforced;
}

/* ============================================================
 * AI_TACTICAL_EXECUTE — Main entry point for tactical phase
 * ============================================================ */

int
ai_tactical_execute(TACTICAL_STATE_PTR state)
{
    int total_actions = 0;
    int target_count = 0;

    if (state == NULL) return 0;

#ifdef MEMORYH
    fprintf(fupdate,
            "  TAC: Nation %d tactical phase — attack=%d%% retreat=%d%% garrison=%d\n",
            state->nation_id,
            state->attack_ratio,
            state->retreat_ratio,
            state->garrison_threshold);

    /* Phase 1: Evaluate targets */
    TACTICAL_TARGET_PTR targets = ai_tactical_evaluate(state, &target_count);
    if (targets == NULL || target_count == 0) {
        fprintf(fupdate,
                "  TAC: Nation %d — no tactical targets\n",
                state->nation_id);
        return 0;
    }

    /* Phase 2: Attack promising targets */
    TACTICAL_TARGET_PTR tgt = targets;
    while (tgt != NULL) {
        if (tgt->type == TAC_ATTACK && ai_should_attack(tgt->x, tgt->y, state)) {
            /* Find an army to send */
            for (army_ptr = ntn_ptr->army_list;
                 army_ptr != NULL;
                 army_ptr = army_ptr->next) {
                if (ARMY_STAT == ST_GARRISON) continue;
                int dist = abs(tgt->x - ARMY_XLOC) + abs(tgt->y - ARMY_YLOC);
                if (dist <= 3) { /* Only send nearby armies */
                    set_status(ARMY_STAT, ST_ATTACK);
                    state->attacks_launched++;
                    total_actions++;
                    fprintf(fupdate,
                            "  TAC: Nation %d attacks (%d,%d) est_enemy=%d ratio=%d\n",
                            state->nation_id, tgt->x, tgt->y,
                            tgt->estimated_enemy, state->attack_ratio);
                    break;
                }
            }
        } else if (tgt->type == TAC_RETREAT) {
            /* Check if any of our armies are in danger here */
            for (army_ptr = ntn_ptr->army_list;
                 army_ptr != NULL;
                 army_ptr = army_ptr->next) {
                if (ARMY_XLOC == tgt->x && ARMY_YLOC == tgt->y) {
                    if (ai_should_retreat(tgt->x, tgt->y, state)) {
                        /* Move toward nearest owned sector */
                        int best_x = -1, best_y = -1;
                        int best_dist = 9999;
                        int sx, sy;
                        for (sx = ntn_ptr->leftedge; sx <= ntn_ptr->rightedge; sx++) {
                            for (sy = ntn_ptr->topedge; sy <= ntn_ptr->bottomedge; sy++) {
                                if (!XY_ONMAP(sx, sy)) continue;
                                if (sct[sx][sy].owner != state->nation_id) continue;
                                int d = abs(sx - tgt->x) + abs(sy - tgt->y);
                                if (d < best_dist && d > 0) {
                                    best_dist = d;
                                    best_x = sx;
                                    best_y = sy;
                                }
                            }
                        }
                        if (best_x >= 0 && npc_movearmy(best_x, best_y)) {
                            state->retreats_ordered++;
                            total_actions++;
                            fprintf(fupdate,
                                    "  TAC: Nation %d retreats from (%d,%d) to (%d,%d)\n",
                                    state->nation_id, tgt->x, tgt->y, best_x, best_y);
                        }
                        break;
                    }
                }
            }
        }
        tgt = tgt->next;
    }

    /* Phase 3: Garrison placement */
    int garrisons = ai_place_garrisons(state);
    total_actions += garrisons;

    /* Phase 4: Reinforce threatened borders */
    int reinforced = ai_reinforce_borders(state);
    total_actions += reinforced;

    /* Summary */
    fprintf(fupdate,
            "  TAC: Nation %d tactical phase complete — "
            "attacks=%d retreats=%d garrisons=%d reinforced=%d\n",
            state->nation_id,
            state->attacks_launched,
            state->retreats_ordered,
            garrisons,
            reinforced);

    ai_tactical_free_targets(targets);
#endif

    return total_actions;
}

/* ============================================================
 * AI_TACTICAL_FREE_TARGETS — Free target list memory
 * ============================================================ */

void
ai_tactical_free_targets(TACTICAL_TARGET_PTR list)
{
    while (list != NULL) {
        TACTICAL_TARGET_PTR next = list->next;
        free(list);
        list = next;
    }
}