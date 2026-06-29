// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * action_expand.h - AI Expand Action Module for Conquer V5
 * Sprint 1 Task 1.4: Real Action Execution
 *
 * This module handles territory claiming, military movement,
 * and sector construction for NPC nations. Decisions are
 * personality-weighted and fog-of-war constrained (Honest AI).
 *
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */

#ifndef ACTION_EXPAND_H
#define ACTION_EXPAND_H

/* Note: action_expand.c includes dataA.h before this header.
 * dataA.h provides all game data structures, macros, and globals.
 * Only include supplemental headers not already in dataA.h's chain. */

/* ============================================================
 * Expand Action Result Codes
 * ============================================================ */

typedef enum {
  EXPAND_OK = 0,          /* Action completed successfully */
  EXPAND_NO_MOVEMENT,     /* No movement possible */
  EXPAND_NO_TARGETS,       /* No valid targets found */
  EXPAND_NO_TROOPS,       /* No troops available for action */
  EXPAND_FOG_BLOCKED,     /* Target not visible through fog */
  EXPAND_RESOURCE_LOW,    /* Not enough resources */
  EXPAND_ALREADY_OWNED,   /* Sector already owned by nation */
  EXPAND_ENEMY_TOO_STRONG,/* Enemy forces exceed threshold */
  EXPAND_NO_ADJACENT,     /* No adjacent owned sector to expand from */
  EXPAND_ERROR             /* General error */
} expand_result_t;

/* ============================================================
 * Target Priority Types
 * ============================================================ */

typedef enum {
  TGT_CLAIM_EMPTY,    /* Claim unowned adjacent territory */
  TGT_CLAIM_CONTESTED,/* Contest enemy-held territory */
  TGT_REINFORCE,      /* Move to reinforce border sector */
  TGT_ATTACK,          /* Attack enemy position */
  TGT_RETREAT,         /* Pull back to safer position */
  TGT_PATROL           /* Move along border (roving) */
} target_type_t;

/* ============================================================
 * Expand Target Structure
 * ============================================================ */

typedef struct s_expand_target {
  int x, y;               /* Target coordinates */
  target_type_t type;     /* Type of target */
  int priority;           /* Priority value (0-100, higher = more urgent) */
  int estimated_strength; /* Estimated enemy strength at target (0 if unknown) */
  struct s_expand_target *next;
} EXPAND_TARGET, *EXPAND_TARGET_PTR;

/* ============================================================
 * Expand Action State
 * ============================================================ */

typedef struct s_expand_state {
  /* Personality weights (from PERSONALITY_STRUCT, copied here for fast access) */
  int weight_military;     /* Military aggression weight */
  int weight_economy;      /* Economic development weight */
  int weight_defense;      /* Defense/garrison weight */
  int weight_expansion;    /* Territory expansion weight */

  /* Nation context */
  int nation_id;           /* Our nation id (country) */
  int turn_number;         /* Current game turn */

  /* Combat thresholds */
  int attack_threshold;    /* Min strength advantage ratio to attack (e.g., 120 = need 20% more) */
  int retreat_threshold;   /* Retreat if enemy exceeds this ratio */

  /* Build preferences (personality-weighted) */
  int pref_fortify;        /* Preference for fortification */
  int pref_economy;        /* Preference for economic building */
  int pref_military;       /* Preference for military infrastructure */

  /* Tracking */
  int sectors_claimed;     /* Sectors claimed this turn */
  int armies_moved;       /* Armies moved this turn */
  int buildings_started;  /* Buildings started this turn */
} EXPAND_STATE, *EXPAND_STATE_PTR;

/* ============================================================
 * Personality → Default Thresholds
 * ============================================================ */

/* Combat threshold defaults by personality type.
 * These are starting points; can be tuned per-nation. */

#define ATTACK_THRESH_WARLORD    110  /* Warlord attacks with just 10% advantage */
#define ATTACK_THRESH_PIONEER    130  /* Pioneer waits for 30% advantage */
#define ATTACK_THRESH_MERCHANT   150  /* Merchant cautious, needs 50% advantage */
#define ATTACK_THRESH_STRATEGIST 120  /* Strategist attacks at 20% advantage */
#define ATTACK_THRESH_FORTRESS    200 /* Fortress rarely attacks, 2:1 needed */

#define RETREAT_THRESH_WARLORD     40  /* Warlord retreats only if <40% strength */
#define RETREAT_THRESH_PIONEER     60  /* Pioneer retreats at 60% */
#define RETREAT_THRESH_MERCHANT    75  /* Merchant retreats at 75% */
#define RETREAT_THRESH_STRATEGIST   50  /* Strategist retreats at 50% (calculated risk) */
#define RETREAT_THRESH_FORTRESS     80  /* Fortress retreats at 80% (conservative) */

/* Build preference defaults by personality */
#define BUILDPREF_WARLORD    {3, 1, 5}  /* fortify, economy, military */
#define BUILDPREF_PIONEER    {1, 2, 2}
#define BUILDPREF_MERCHANT   {0, 5, 1}
#define BUILDPREF_STRATEGIST  {3, 3, 3}
#define BUILDPREF_FORTRESS   {5, 1, 3}

/* ============================================================
 * Function Prototypes
 * ============================================================ */

/* Initialize expand state from personality and fog data */
expand_result_t action_expand_init(EXPAND_STATE_PTR state, int nation_id);

/* Main entry point: execute all expand actions for current nation */
expand_result_t action_expand_execute(EXPAND_STATE_PTR state);

/* Claim unowned/contested territory adjacent to borders */
int ai_claim_sector(EXPAND_STATE_PTR state);

/* Strategic military movement: reinforce, advance, retreat */
int ai_move_military(EXPAND_STATE_PTR state);

/* Construct improvements in owned territory */
int ai_build_in_sector(EXPAND_STATE_PTR state);

/* Target evaluation: build priority list of sectors to act on */
EXPAND_TARGET_PTR ai_evaluate_targets(EXPAND_STATE_PTR state, int *target_count);

/* Free target list memory */
void ai_free_targets(EXPAND_TARGET_PTR list);

/* Sector value assessment (personality-weighted) */
int ai_sector_priority(int x, int y, EXPAND_STATE_PTR state);

/* Check if a sector is visible to this nation through fog-of-war */
int ai_sector_visible(int x, int y, int nation_id);

/* Count friendly military strength at/near a sector */
long ai_friendly_strength(int x, int y, int nation_id, int range);

/* Estimate enemy strength at a sector (from fog data) */
long ai_enemy_strength(int x, int y, int nation_id);

/* Determine best designation for a sector based on personality */
int ai_best_designation(int x, int y, EXPAND_STATE_PTR state);

/* Check if nation owns at least one adjacent sector */
int ai_has_adjacent_owned(int x, int y, int nation_id);

/* Check if nation owns any territory at all */
int ai_has_any_territory(int nation_id);

#endif /* ACTION_EXPAND_H */