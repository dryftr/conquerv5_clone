/* ai_economic.h — Economic AI Module for Conquer V5
 * Sprint 2 Task 2.3: Build prioritization, resource balancing, construction queues
 *
 * All values are personality-weighted and difficulty-scaled.
 * JSON-driven: modders can tweak balance by editing personality files.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */
#ifndef AI_ECONOMIC_H
#define AI_ECONOMIC_H

#ifdef MEMORYH
#include "dataA.h"
#endif

/* ============================================================
 * Build Category Types
 * ============================================================ */

typedef enum {
    BUILD_MILITARY,      /* Military units and infrastructure */
    BUILD_FORTIFICATION, /* Walls, towers, defensive structures */
    BUILD_ECONOMY,       /* Markets, farms, resource production */
    BUILD_NAVAL,         /* Ships, ports, naval infrastructure */
    BUILD_CARAVAN,       /* Trade routes, caravans */
    BUILD_COUNT          /* Total build categories */
} build_category_t;

/* ============================================================
 * Economic Target Structure
 * ============================================================ */

typedef struct s_econ_target {
    int x, y;                    /* Sector coordinates */
    build_category_t category;   /* What to build */
    int base_score;               /* Personality-weighted base score */
    int situational_bonus;       /* Context-dependent modifier */
    int final_score;             /* base_score + situational_bonus */
    long cost;                   /* Estimated gold cost */
    const char *name;            /* Build name (for logging) */
} ECON_TARGET, *ECON_TARGET_PTR;

/* ============================================================
 * Economic State Structure
 * ============================================================ */

typedef struct s_econ_state {
    /* Nation context */
    int nation_id;                /* Our nation id */
    int turn_number;              /* Current game turn */

    /* Economy metrics */
    long treasury;                /* Current gold reserves */
    long income;                  /* Per-turn income */
    long expenses;                /* Per-turn expenses */
    long net_income;             /* income - expenses */

    /* Personality-driven parameters (loaded from JSON) */
    int max_builds_per_turn;      /* Max buildings we can start per turn */
    double reserve_pct;           /* Fraction of treasury to reserve (0.0-1.0) */
    long reserve_amount;          /* reserve_pct * treasury (computed) */
    long spend_budget;            /* treasury - reserve_amount */

    /* Build preference weights (0.0-1.0, from personality) */
    double weight_military;       /* preference for military builds */
    double weight_fortification;  /* preference for defensive builds */
    double weight_economy;        /* preference for economic builds */
    double weight_naval;          /* preference for naval builds */
    double weight_caravan;        /* preference for trade builds */

    /* Situational modifiers (computed per-turn) */
    int under_attack;             /* 1 if we're being attacked this turn */
    int low_troops;               /* 1 if military strength is below threshold */
    int deficit;                  /* 1 if net_income < 0 */

    /* Difficulty multipliers (from DIFFICULTY_CONFIG) */
    double economy_mult;          /* multiplier on economic efficiency */
    double build_cap_mult;        /* multiplier on max_builds_per_turn */

    /* Tracking */
    int builds_started;           /* Number of builds started this turn */
    int builds_skipped;           /* Number skipped (insufficient funds) */
    int builds_completed;         /* Number completed from prior turns */
} ECON_STATE, *ECON_STATE_PTR;

/* ============================================================
 * Build Priority Thresholds (defaults, overridden by JSON)
 * ============================================================ */

/* Default max builds per turn by personality type */
#define ECON_MAX_BUILDS_WARLORD    3
#define ECON_MAX_BUILDS_PIONEER    4
#define ECON_MAX_BUILDS_STRATEGIST 4
#define ECON_MAX_BUILDS_MERCHANT   5
#define ECON_MAX_BUILDS_FORTRESS   4

/* Default reserve percentages (fraction of treasury to keep) */
#define ECON_RESERVE_WARLORD       0.10
#define ECON_RESERVE_PIONEER       0.20
#define ECON_RESERVE_STRATEGIST    0.25
#define ECON_RESERVE_MERCHANT      0.30
#define ECON_RESERVE_FORTRESS      0.15

/* Situational modifier values */
#define ECON_MOD_UNDER_ATTACK      20   /* +20 to fortification when attacked */
#define ECON_MOD_LOW_TROOPS        15   /* +15 to military when troops low */
#define ECON_MOD_DEFICIT           30   /* +30 to economy when in deficit */
#define ECON_MOD_STRONG_ECONOMY    10   /* +10 to military when economy strong */

/* ============================================================
 * Function Prototypes
 * ============================================================ */

/* Initialize economic state from personality data */
int ai_economic_init(ECON_STATE_PTR state, int nation_id);

/* Evaluate the nation's economy (treasury, income, expenses) */
int ai_economic_evaluate(ECON_STATE_PTR state);

/* Score and prioritize build targets for this turn */
int ai_economic_score_builds(ECON_STATE_PTR state,
                              ECON_TARGET_PTR targets, int max_targets);

/* Execute economic decisions: start builds up to budget */
int ai_economic_execute(ECON_STATE_PTR state);

/* Get build category name for logging */
const char *ai_econ_category_name(build_category_t cat);

#endif /* AI_ECONOMIC_H */