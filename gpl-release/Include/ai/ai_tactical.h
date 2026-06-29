/* ai_tactical.h — Tactical AI Module for Conquer V5
 * Sprint 2 Task 2.2: Combat engagement, retreat, garrison, reinforcement
 *
 * The tactical module handles turn-level combat decisions:
 * - Should we attack an enemy position?
 * - Should we retreat from an unfavorable engagement?
 * - Where do we place garrisons on borders?
 * - How do we reinforce threatened sectors?
 *
 * All decisions are personality-weighted and fog-of-war constrained.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */
#ifndef AI_TACTICAL_H
#define AI_TACTICAL_H

/* Include game data types when building with game headers.
 * In standalone mode, types used here (int, etc.) are built-in C types,
 * so no extra include is needed. */
#ifdef MEMORYH
#include "dataA.h"
#endif

/* ============================================================
 * Tactical Target Types
 * ============================================================ */

typedef enum {
    TAC_ATTACK,      /* Engage enemy position */
    TAC_DEFEND,      /* Hold current position */
    TAC_RETREAT,     /* Pull back to safer ground */
    TAC_REINFORCE,   /* Move reserves to threatened sector */
    TAC_SCOUT,       /* Scout unknown or fogged territory */
    TAC_GARRISON     /* Station troops at border sector */
} tactical_type_t;

/* ============================================================
 * Tactical Target Structure
 * ============================================================ */

typedef struct s_tactical_target {
    int x, y;                    /* Target coordinates */
    tactical_type_t type;         /* Type of tactical action */
    int priority;                 /* Priority (0-100, higher = more urgent) */
    int estimated_enemy;          /* Estimated enemy strength (0 if unknown) */
    int estimated_friendly;       /* Estimated friendly strength in range */
    int confidence;               /* Confidence in estimate (0-100, based on fog freshness) */
    struct s_tactical_target *next;
} TACTICAL_TARGET, *TACTICAL_TARGET_PTR;

/* ============================================================
 * Tactical Result Structure
 * ============================================================ */

typedef struct s_tactical_result {
    int attacks_launched;         /* Number of attack orders issued */
    int defenses_ordered;         /* Number of hold-position orders */
    int retreats_ordered;         /* Number of retreat orders */
    int reinforcements_sent;     /* Number of reserve movements */
    int scouts_sent;              /* Number of scouting missions */
    int garrisons_placed;         /* Number of garrison assignments */
    int armies_moved;             /* Total armies with new orders */
} TACTICAL_RESULT, *TACTICAL_RESULT_PTR;

/* ============================================================
 * Tactical State Structure
 * ============================================================ */

typedef struct s_tactical_state {
    /* Nation context */
    int nation_id;                /* Our nation id */
    int turn_number;              /* Current game turn */

    /* Combat thresholds (personality-weighted) */
    int attack_ratio;             /* Min friendly:enemy ratio to attack (%) */
    int retreat_ratio;            /* Retreat if below this ratio (%) */
    int garrison_threshold;       /* Min threat level to place garrison (0-100) */

    /* Personality weights */
    int aggression;               /* 0-100: how aggressively we engage */
    int caution;                  /* 0-100: how quickly we retreat */
    int border_focus;             /* 0-100: priority on border defense */

    /* Tracking */
    int attacks_launched;
    int defenses_ordered;
    int retreats_ordered;
    int reinforcements_sent;
    int garrisons_placed;
} TACTICAL_STATE, *TACTICAL_STATE_PTR;

/* ============================================================
 * Combat Engagement Thresholds
 * ============================================================ */

/* Attack ratio: friendly strength must be this % of enemy to attack.
 * 100 = equal strength, 120 = 20% advantage needed, etc.
 * Lower values = more aggressive. */

#define TAC_ATTACK_WARLORD      100   /* Attack at parity */
#define TAC_ATTACK_PIONEER      120   /* Need 20% advantage */
#define TAC_ATTACK_STRATEGIST   115   /* Need 15% advantage */
#define TAC_ATTACK_MERCHANT     150   /* Need 50% advantage */
#define TAC_ATTACK_FORTRESS     200   /* Need 2:1 advantage (defense-first) */

/* Retreat ratio: if friendly strength drops below this % of enemy, retreat.
 * Higher values = more cautious. */

#define TAC_RETREAT_WARLORD      30   /* Only retreat if <30% strength */
#define TAC_RETREAT_PIONEER      50   /* Retreat at 50% */
#define TAC_RETREAT_STRATEGIST   40   /* Retreat at 40% (calculated risk) */
#define TAC_RETREAT_MERCHANT     65   /* Retreat at 65% (protect economy) */
#define TAC_RETREAT_FORTRESS     75   /* Retreat at 75% (preserve defenses) */

/* Garrison threshold: minimum threat level to station garrison.
 * Higher values = fewer garrisons (more mobile). */

#define TAC_GARRISON_WARLORD     30   /* Garrison most borders */
#define TAC_GARRISON_PIONEER     50   /* Garrison key borders */
#define TAC_GARRISON_STRATEGIST  40   /* Garrison strategic borders */
#define TAC_GARRISON_MERCHANT    60   /* Minimal garrisons */
#define TAC_GARRISON_FORTRESS    20   /* Garrison ALL borders (defense-first) */

/* ============================================================
 * Function Prototypes
 * ============================================================ */

/* Initialize tactical state from personality data */
int ai_tactical_init(TACTICAL_STATE_PTR state, int nation_id);

/* Evaluate all tactical targets for this nation this turn */
TACTICAL_TARGET_PTR ai_tactical_evaluate(TACTICAL_STATE_PTR state, int *target_count);

/* Execute tactical decisions: attacks, retreats, garrisons, reinforcements */
int ai_tactical_execute(TACTICAL_STATE_PTR state);

/* Determine if we should attack a specific target */
int ai_should_attack(int x, int y, TACTICAL_STATE_PTR state);

/* Determine if we should retreat from a position */
int ai_should_retreat(int x, int y, TACTICAL_STATE_PTR state);

/* Place garrisons on border sectors */
int ai_place_garrisons(TACTICAL_STATE_PTR state);

/* Move reserve armies to reinforce threatened sectors */
int ai_reinforce_borders(TACTICAL_STATE_PTR state);

/* Free tactical target list memory */
void ai_tactical_free_targets(TACTICAL_TARGET_PTR list);

#endif /* AI_TACTICAL_H */