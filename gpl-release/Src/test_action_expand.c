// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * test_action_expand.c - Test suite for AI Expand Action Module
 * Sprint 1 Task 1.4c
 *
 * Key: Nation ID 0 = UNOWNED. Our AI = nation 1, enemy = nation 2.
 *
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dataA.h"
#include "armyX.h"
#include "moveX.h"
#include "activeX.h"
#include "desigX.h"
#include "statusX.h"
#include "executeX.h"
#include "action_expand.h"

/* Our AI nation is ID 1 (0 = UNOWNED in Conquer) */
#define AI_NATION 1
#define ENEMY_NATION 2

/* ============================================================
 * Test framework
 * ============================================================ */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_EQ(a, b, msg) do { \
  tests_run++; \
  if ((a) != (b)) { \
    tests_failed++; \
    printf("  TEST %d: %s ... FAIL: expected %d, got %d\n", tests_run, msg, (int)(b), (int)(a)); \
  } else { \
    tests_passed++; \
    printf("  TEST %d: %s ... PASS\n", tests_run, msg); \
  } \
} while (0)

#define ASSERT_TRUE(cond, msg) do { \
  tests_run++; \
  if (!(cond)) { \
    tests_failed++; \
    printf("  TEST %d: %s ... FAIL: %s\n", tests_run, msg, #cond); \
  } else { \
    tests_passed++; \
    printf("  TEST %d: %s ... PASS\n", tests_run, msg); \
  } \
} while (0)

#define ASSERT_FALSE(cond, msg) ASSERT_TRUE(!(cond), msg)

/* ============================================================
 * Mock setup helpers
 * ============================================================ */

extern void test_init_world(void);
extern void test_cleanup_world(void);
extern SCT_STRUCT **sct;
extern struct s_world world;
extern NTN_PTR ntn_ptr;

#define MOCK_MAPX 20
#define MOCK_MAPY 20

static void
reset_world(void)
{
  test_cleanup_world();
  test_init_world();
}

/* Give AI nation territory */
static void
setup_ai_territory(void)
{
  int x, y;
  for (x = 3; x <= 7; x++) {
    for (y = 3; y <= 7; y++) {
      sct[x][y].owner = AI_NATION;
      sct[x][y].designation = MAJ_FARM;
      sct[x][y].efficiency = 50;
      sct[x][y].people = 100;
    }
  }
}

/* Give enemy nation territory */
static void
setup_enemy_territory(void)
{
  int x, y;
  for (x = 14; x <= 17; x++) {
    for (y = 14; y <= 17; y++) {
      sct[x][y].owner = ENEMY_NATION;
      sct[x][y].designation = MAJ_FARM;
      sct[x][y].efficiency = 50;
    }
  }
  world.np[ENEMY_NATION]->active = ACT_OVERT;
}

#define MOCK_ARMY_COUNT 5
static ARMY_STRUCT mock_armies[MOCK_ARMY_COUNT];

static void
setup_ai_armies(void)
{
  int i;
  for (i = 0; i < MOCK_ARMY_COUNT; i++) {
    memset(&mock_armies[i], 0, sizeof(ARMY_STRUCT));
    mock_armies[i].armyid = i + 1;
    mock_armies[i].strength = 100;
    mock_armies[i].xloc = 5 + i;
    mock_armies[i].yloc = 5;
    mock_armies[i].next = (i < MOCK_ARMY_COUNT - 1) ? &mock_armies[i + 1] : NULL;
  }
  ntn_ptr->army_list = &mock_armies[0];
}

static void
clear_ai_armies(void)
{
  ntn_ptr->army_list = NULL;
}

/* ============================================================
 * Tests: Personality Thresholds
 * ============================================================ */

static void test_init_killer(void)
{
  EXPAND_STATE state;
  expand_result_t r = action_expand_init(&state, AI_NATION);
  ASSERT_EQ(r, EXPAND_OK, "Killer init should succeed");
  ASSERT_EQ(state.attack_threshold, ATTACK_THRESH_WARLORD, "Killer attack threshold");
  ASSERT_EQ(state.retreat_threshold, RETREAT_THRESH_WARLORD, "Killer retreat threshold");
  ASSERT_EQ(state.pref_military, 5, "Killer military pref");
  ASSERT_EQ(state.pref_economy, 1, "Killer economy pref");
}

static void test_init_null(void)
{
  expand_result_t r = action_expand_init(NULL, AI_NATION);
  ASSERT_EQ(r, EXPAND_ERROR, "NULL init should fail");
}

static void test_init_overt(void)
{
  EXPAND_STATE state;
  world.np[AI_NATION]->active = ACT_OVERT;
  expand_result_t r = action_expand_init(&state, AI_NATION);
  ASSERT_EQ(r, EXPAND_OK, "Overt init should succeed");
  ASSERT_EQ(state.attack_threshold, ATTACK_THRESH_PIONEER, "Overt attack threshold");
  ASSERT_TRUE(state.weight_expansion > state.weight_defense,
              "Overt should prioritize expansion over defense");
  world.np[AI_NATION]->active = ACT_KILLER;
}

static void test_init_fortress(void)
{
  EXPAND_STATE state;
  world.np[AI_NATION]->active = ACT_ENFORCE;
  expand_result_t r = action_expand_init(&state, AI_NATION);
  ASSERT_EQ(r, EXPAND_OK, "Fortress init should succeed");
  ASSERT_EQ(state.attack_threshold, ATTACK_THRESH_FORTRESS, "Fortress attack threshold (200)");
  ASSERT_TRUE(state.weight_defense > state.weight_military,
              "Fortress should prioritize defense over military");
  world.np[AI_NATION]->active = ACT_KILLER;
}

static void test_init_static(void)
{
  EXPAND_STATE state;
  world.np[AI_NATION]->active = ACT_STATIC;
  expand_result_t r = action_expand_init(&state, AI_NATION);
  ASSERT_EQ(r, EXPAND_OK, "Static init should succeed");
  ASSERT_EQ(state.attack_threshold, 250, "Static attack threshold (250)");
  ASSERT_TRUE(state.weight_economy > state.weight_military,
              "Static should prioritize economy over military");
  world.np[AI_NATION]->active = ACT_KILLER;
}

/* ============================================================
 * Tests: Fog-of-War Visibility
 * ============================================================ */

static void test_visible_own_sector(void)
{
  setup_ai_territory();
  ASSERT_TRUE(ai_sector_visible(5, 5, AI_NATION), "Own sector (5,5) should be visible");
}

static void test_visible_adjacent(void)
{
  setup_ai_territory();
  ASSERT_TRUE(ai_sector_visible(2, 5, AI_NATION), "Adjacent (2,5) should be visible");
  ASSERT_TRUE(ai_sector_visible(8, 5, AI_NATION), "Adjacent (8,5) should be visible");
}

static void test_invisible_far(void)
{
  setup_ai_territory();
  ASSERT_FALSE(ai_sector_visible(15, 15, AI_NATION),
               "Far sector (15,15) should NOT be visible to AI");
}

static void test_invisible_offmap(void)
{
  ASSERT_FALSE(ai_sector_visible(-1, 5, AI_NATION), "Offmap should not be visible");
  ASSERT_FALSE(ai_sector_visible(999, 999, AI_NATION), "Offmap should not be visible");
}

/* ============================================================
 * Tests: Adjacent Ownership
 * ============================================================ */

static void test_has_adjacent_owned_yes(void)
{
  setup_ai_territory();
  ASSERT_TRUE(ai_has_adjacent_owned(2, 5, AI_NATION),
              "(2,5) has adjacent AI-owned sector");
}

static void test_has_adjacent_owned_no(void)
{
  /* All unowned — no adjacent owned */
  ASSERT_FALSE(ai_has_adjacent_owned(10, 10, AI_NATION),
               "No adjacent owned when all unowned");
}

/* ============================================================
 * Tests: Sector Priority
 * ============================================================ */

static void test_priority_unowned(void)
{
  EXPAND_STATE state;
  setup_ai_territory();
  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&state, AI_NATION);

  int pri = ai_sector_priority(2, 5, &state);
  ASSERT_TRUE(pri > 0, "Unowned adjacent sector should have positive priority");
}

static void test_priority_owned_lower(void)
{
  EXPAND_STATE state;
  setup_ai_territory();
  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&state, AI_NATION);

  int pri_owned = ai_sector_priority(5, 5, &state);
  int pri_unowned = ai_sector_priority(2, 5, &state);
  ASSERT_TRUE(pri_unowned >= pri_owned,
              "Unowned adjacent priority >= owned");
}

static void test_priority_offmap(void)
{
  EXPAND_STATE state;
  action_expand_init(&state, AI_NATION);
  int pri = ai_sector_priority(-1, -1, &state);
  ASSERT_EQ(pri, 0, "Offmap priority should be 0");
}

static void test_priority_with_minerals(void)
{
  EXPAND_STATE state;
  setup_ai_territory();
  sct[2][5].minerals = 8;
  sct[2][5].owner = UNOWNED;
  world.np[AI_NATION]->active = ACT_OVERT;
  action_expand_init(&state, AI_NATION);

  int pri_mineral = ai_sector_priority(2, 5, &state);
  sct[2][5].minerals = 0;
  int pri_plain = ai_sector_priority(2, 5, &state);
  ASSERT_TRUE(pri_mineral > pri_plain, "Mineral sector should have higher priority");
}

/* ============================================================
 * Tests: Target Evaluation
 * ============================================================ */

static void test_targets_empty_map(void)
{
  reset_world();
  EXPAND_STATE state;
  action_expand_init(&state, AI_NATION);
  int count = 0;
  EXPAND_TARGET_PTR targets = ai_evaluate_targets(&state, &count);
  ASSERT_TRUE(count == 0 || targets == NULL,
              "Empty map should produce no targets");
  if (targets) ai_free_targets(targets);
}

static void test_targets_with_territory(void)
{
  EXPAND_STATE state;
  setup_ai_territory();
  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&state, AI_NATION);

  int count = 0;
  EXPAND_TARGET_PTR targets = ai_evaluate_targets(&state, &count);
  ASSERT_TRUE(count > 0, "Map with territory should produce targets");
  ASSERT_TRUE(targets != NULL, "Targets list should not be null");

  if (targets && targets->next) {
    ASSERT_TRUE(targets->priority >= targets->next->priority,
                "Targets should be sorted descending by priority");
  }

  ai_free_targets(targets);
}

/* ============================================================
 * Tests: Sector Claiming
 * ============================================================ */

static void test_claim_adjacent(void)
{
  EXPAND_STATE state;
  setup_ai_territory();
  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&state, AI_NATION);

  fupdate = tmpfile();
  fexe = tmpfile();

  int claimed = ai_claim_sector(&state);
  ASSERT_TRUE(claimed > 0, "Should claim at least one sector");
  ASSERT_TRUE(claimed < 50, "Should not claim too many sectors");

  /* Verify a border sector was claimed by AI_NATION (1), not UNOWNED (0) */
  ASSERT_TRUE(sct[2][5].owner == AI_NATION || sct[8][5].owner == AI_NATION ||
              sct[5][2].owner == AI_NATION || sct[5][8].owner == AI_NATION,
              "At least one border sector should be claimed by AI");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

static void test_claim_no_territory(void)
{
  reset_world();
  EXPAND_STATE state;
  action_expand_init(&state, AI_NATION);

  fupdate = tmpfile();
  fexe = tmpfile();

  int claimed = ai_claim_sector(&state);
  ASSERT_EQ(claimed, 0, "Should claim 0 with no adjacent territory");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

/* ============================================================
 * Tests: Military Movement
 * ============================================================ */

static void test_move_with_armies(void)
{
  EXPAND_STATE state;
  setup_ai_territory();
  setup_enemy_territory();
  setup_ai_armies();
  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&state, AI_NATION);

  fupdate = tmpfile();
  fexe = tmpfile();

  int moved = ai_move_military(&state);
  ASSERT_TRUE(moved >= 0, "move_military should return non-negative");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

static void test_move_no_armies(void)
{
  EXPAND_STATE state;
  setup_ai_territory();
  clear_ai_armies();
  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&state, AI_NATION);

  fupdate = tmpfile();
  fexe = tmpfile();

  int moved = ai_move_military(&state);
  ASSERT_EQ(moved, 0, "Should move 0 armies with no army list");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

/* ============================================================
 * Tests: Best Designation
 * ============================================================ */

static void test_designation_minerals(void)
{
  EXPAND_STATE state;
  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&state, AI_NATION);

  sct[10][10].minerals = 8;
  sct[10][10].owner = AI_NATION;

  int desg = ai_best_designation(10, 10, &state);
  ASSERT_EQ(desg, MAJ_METALMINE, "High mineral sector should be mine");
}

static void test_designation_trade_good(void)
{
  EXPAND_STATE state;
  world.np[AI_NATION]->active = ACT_STATIC;
  action_expand_init(&state, AI_NATION);

  sct[10][10].tradegood = 1;
  sct[10][10].people = 150;
  sct[10][10].minerals = 0;
  sct[10][10].owner = AI_NATION;

  int desg = ai_best_designation(10, 10, &state);
  ASSERT_TRUE(desg >= MAJ_NONE && desg < MAJ_NUMBER, "Designation should be valid");
}

static void test_designation_border_stockade(void)
{
  EXPAND_STATE state;
  setup_ai_territory();
  setup_enemy_territory();
  world.np[AI_NATION]->active = ACT_ENFORCE;
  action_expand_init(&state, AI_NATION);

  /* Border sector near enemy */
  sct[8][5].owner = AI_NATION;
  sct[9][5].owner = ENEMY_NATION;
  sct[8][5].efficiency = 30;

  int desg = ai_best_designation(8, 5, &state);
  ASSERT_TRUE(desg >= MAJ_NONE, "Should return valid designation near enemy");
}

static void test_designation_default_farm(void)
{
  EXPAND_STATE state;
  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&state, AI_NATION);

  sct[10][10].owner = AI_NATION;
  sct[10][10].people = 10;
  sct[10][10].minerals = 0;
  sct[10][10].tradegood = 0;
  sct[10][10].efficiency = 30;

  int desg = ai_best_designation(10, 10, &state);
  ASSERT_TRUE(desg == MAJ_FARM || desg == MAJ_STOCKADE,
              "Low-value sector should default to farm or stockade");
}

/* ============================================================
 * Tests: Strength Assessment
 * ============================================================ */

static void test_friendly_strength_none(void)
{
  clear_ai_armies();
  long str = ai_friendly_strength(5, 5, AI_NATION, 2);
  ASSERT_EQ(str, 0, "No armies = 0 friendly strength");
}

static void test_friendly_strength_with_armies(void)
{
  setup_ai_armies();
  long str = ai_friendly_strength(7, 5, AI_NATION, 5);
  ASSERT_TRUE(str > 0, "Should detect friendly strength nearby");
}

static void test_enemy_strength_invisible(void)
{
  setup_ai_territory();
  long str = ai_enemy_strength(15, 15, AI_NATION);
  ASSERT_EQ(str, 0, "Invisible sector should report 0 enemy strength");
}

/* ============================================================
 * Tests: Full Execute Pipeline
 * ============================================================ */

static void test_execute_no_territory(void)
{
  reset_world();
  EXPAND_STATE state;
  action_expand_init(&state, AI_NATION);

  fupdate = tmpfile();
  fexe = tmpfile();

  expand_result_t r = action_expand_execute(&state);
  ASSERT_EQ(r, EXPAND_NO_TARGETS, "No territory → NO_TARGETS");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

static void test_execute_with_territory(void)
{
  EXPAND_STATE state;
  setup_ai_territory();
  setup_ai_armies();
  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&state, AI_NATION);

  fupdate = tmpfile();
  fexe = tmpfile();

  expand_result_t r = action_expand_execute(&state);
  ASSERT_TRUE(r == EXPAND_OK || r == EXPAND_NO_TARGETS,
              "Expand should succeed or find no targets");
  ASSERT_TRUE(state.sectors_claimed >= 0, "Claimed >= 0");
  ASSERT_TRUE(state.armies_moved >= 0, "Moved >= 0");
  ASSERT_TRUE(state.buildings_started >= 0, "Built >= 0");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

/* ============================================================
 * Tests: Memory Safety
 * ============================================================ */

static void test_free_null_targets(void)
{
  ai_free_targets(NULL);
  ASSERT_TRUE(1, "free_targets(NULL) should not crash");
}

/* ============================================================
 * Tests: Personality Variance
 * ============================================================ */

static void test_killer_vs_static(void)
{
  EXPAND_STATE killer, stat;

  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&killer, AI_NATION);

  world.np[AI_NATION]->active = ACT_STATIC;
  action_expand_init(&stat, AI_NATION);

  ASSERT_TRUE(killer.attack_threshold < stat.attack_threshold,
              "Killer more aggressive than Static");
  ASSERT_TRUE(killer.weight_military > stat.weight_military,
              "Killer has higher military weight");
  ASSERT_TRUE(killer.weight_defense < stat.weight_defense,
              "Killer has lower defense weight");
}

static void test_killer_vs_fortress(void)
{
  EXPAND_STATE killer, fortress;

  world.np[AI_NATION]->active = ACT_KILLER;
  action_expand_init(&killer, AI_NATION);

  world.np[AI_NATION]->active = ACT_ENFORCE;
  action_expand_init(&fortress, AI_NATION);

  ASSERT_TRUE(killer.attack_threshold < fortress.attack_threshold,
              "Killer attacks more aggressively than Fortress");
  ASSERT_TRUE(fortress.weight_defense > fortress.weight_military,
              "Fortress prioritizes defense over offense");
}

/* ============================================================
 * Main
 * ============================================================ */

int
main(int argc, char *argv[])
{
  printf("=== Action Expand Test Suite (Sprint 1.4c) ===\n\n");

  test_init_world();

  printf("--- Personality Threshold Tests ---\n");
  test_init_killer();
  test_init_null();
  test_init_overt();
  test_init_fortress();
  test_init_static();

  printf("\n--- Fog-of-War Visibility Tests ---\n");
  test_visible_own_sector();
  test_visible_adjacent();
  test_invisible_far();
  test_invisible_offmap();

  printf("\n--- Adjacent Ownership Tests ---\n");
  test_has_adjacent_owned_yes();
  test_has_adjacent_owned_no();

  printf("\n--- Sector Priority Tests ---\n");
  test_priority_unowned();
  test_priority_owned_lower();
  test_priority_offmap();
  test_priority_with_minerals();

  printf("\n--- Target Evaluation Tests ---\n");
  test_targets_empty_map();
  test_targets_with_territory();

  printf("\n--- Sector Claiming Tests ---\n");
  test_claim_adjacent();
  test_claim_no_territory();

  printf("\n--- Military Movement Tests ---\n");
  test_move_with_armies();
  test_move_no_armies();

  printf("\n--- Designation Tests ---\n");
  test_designation_minerals();
  test_designation_trade_good();
  test_designation_border_stockade();
  test_designation_default_farm();

  printf("\n--- Strength Assessment Tests ---\n");
  test_friendly_strength_none();
  test_friendly_strength_with_armies();
  test_enemy_strength_invisible();

  printf("\n--- Full Execute Tests ---\n");
  test_execute_no_territory();
  test_execute_with_territory();

  printf("\n--- Memory Safety Tests ---\n");
  test_free_null_targets();

  printf("\n--- Personality Variance Tests ---\n");
  test_killer_vs_static();
  test_killer_vs_fortress();

  test_cleanup_world();

  printf("\n=== Results: %d passed, %d failed, %d total ===\n",
         tests_passed, tests_failed, tests_run);

  return tests_failed > 0 ? 1 : 0;
}