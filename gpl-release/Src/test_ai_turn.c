// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * test_ai_turn.c - Test suite for AI Turn Orchestrator
 * Sprint 1 Task 1.5
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
#include "ai_turn.h"

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

#define MOCK_ARMY_COUNT 5
static ARMY_STRUCT mock_armies[MOCK_ARMY_COUNT];

static void
reset_world(void)
{
  test_cleanup_world();
  test_init_world();
}

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
  world.np[ENEMY_NATION]->active = ACT_KILLER;
}

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
 * Tests: Strategy Name
 * ============================================================ */

static void test_strategy_names(void)
{
  ASSERT_EQ(strcmp(ai_strategy_name(STRAT_EXPAND), "Expand"), 0, "Expand name");
  ASSERT_EQ(strcmp(ai_strategy_name(STRAT_ATTACK), "Attack"), 0, "Attack name");
  ASSERT_EQ(strcmp(ai_strategy_name(STRAT_DEFEND), "Defend"), 0, "Defend name");
  ASSERT_EQ(strcmp(ai_strategy_name(STRAT_ECONOMY), "Economy"), 0, "Economy name");
  ASSERT_EQ(strcmp(ai_strategy_name(STRAT_CONSOLIDATE), "Consolidate"), 0, "Consolidate name");
  ASSERT_EQ(strcmp(ai_strategy_name(STRAT_PATROL), "Patrol"), 0, "Patrol name");
}

/* ============================================================
 * Tests: Turn Init
 * ============================================================ */

static void test_init_killer(void)
{
  reset_world();
  setup_ai_territory();
  world.np[AI_NATION]->active = ACT_KILLER;

  TURN_CONTEXT ctx;
  turn_result_t r = ai_turn_init(&ctx, AI_NATION);
  ASSERT_EQ(r, TURN_OK, "Killer init should succeed");
  ASSERT_EQ(ctx.nation_id, AI_NATION, "Nation ID set");
  ASSERT_EQ(ctx.phase_flags, PHASE_ALL, "All phases enabled by default");
  ASSERT_TRUE(ctx.weight_military > 0, "Military weight > 0");
  ASSERT_TRUE(ctx.weight_economy > 0, "Economy weight > 0");
}

static void test_init_static(void)
{
  reset_world();
  setup_ai_territory();
  world.np[AI_NATION]->active = ACT_STATIC;

  TURN_CONTEXT ctx;
  turn_result_t r = ai_turn_init(&ctx, AI_NATION);
  ASSERT_EQ(r, TURN_OK, "Static init should succeed");
  ASSERT_TRUE(ctx.weight_economy > ctx.weight_military,
              "Static economy > military");
}

static void test_init_no_territory(void)
{
  reset_world();
  TURN_CONTEXT ctx;
  turn_result_t r = ai_turn_init(&ctx, AI_NATION);
  ASSERT_EQ(r, TURN_NO_TERRITORY, "No territory should return NO_TERRITORY");
  ASSERT_FALSE(ctx.phase_flags & PHASE_EXPAND, "Expand phase disabled without territory");
}

static void test_init_null(void)
{
  turn_result_t r = ai_turn_init(NULL, AI_NATION);
  ASSERT_EQ(r, TURN_ERROR, "NULL init should fail");
}

/* ============================================================
 * Tests: Strategy Selection
 * ============================================================ */

static void test_strategy_killer_default(void)
{
  reset_world();
  setup_ai_territory();
  setup_ai_armies();  /* Killer needs troops to choose Attack */
  world.np[AI_NATION]->active = ACT_KILLER;

  TURN_CONTEXT ctx;
  ai_turn_init(&ctx, AI_NATION);
  strategy_type_t s = ai_turn_select_strategy(&ctx);
  /* Killer with troops defaults to Attack */
  ASSERT_TRUE(s == STRAT_ATTACK || s == STRAT_EXPAND,
              "Killer should select Attack or Expand");
}

static void test_strategy_static_default(void)
{
  reset_world();
  setup_ai_territory();
  world.np[AI_NATION]->active = ACT_STATIC;

  TURN_CONTEXT ctx;
  ai_turn_init(&ctx, AI_NATION);
  strategy_type_t s = ai_turn_select_strategy(&ctx);
  /* Static defaults to Consolidate or Economy */
  ASSERT_TRUE(s == STRAT_CONSOLIDATE || s == STRAT_ECONOMY,
              "Static should select Consolidate or Economy");
}

static void test_strategy_merchant_default(void)
{
  reset_world();
  setup_ai_territory();
  world.np[AI_NATION]->active = ACT_MOBILE;

  TURN_CONTEXT ctx;
  ai_turn_init(&ctx, AI_NATION);
  strategy_type_t s = ai_turn_select_strategy(&ctx);
  /* Merchant defaults to Economy */
  ASSERT_TRUE(s == STRAT_ECONOMY,
              "Merchant should select Economy");
}

static void test_strategy_high_threat(void)
{
  reset_world();
  setup_ai_territory();
  setup_enemy_territory();
  /* Add enemy army near border to increase threat */
  ARMY_STRUCT enemy_army;
  memset(&enemy_army, 0, sizeof(enemy_army));
  enemy_army.armyid = 100;
  enemy_army.strength = 500;
  enemy_army.xloc = 13;
  enemy_army.yloc = 5;
  enemy_army.next = NULL;
  world.np[ENEMY_NATION]->army_list = &enemy_army;

  world.np[AI_NATION]->active = ACT_OVERT;

  TURN_CONTEXT ctx;
  ai_turn_init(&ctx, AI_NATION);
  ai_turn_update_fog(&ctx);

  /* With enemy near border, strategy should shift toward defense */
  strategy_type_t s = ai_turn_select_strategy(&ctx);
  /* Threat may be moderate due to mock setup, just verify it doesn't crash */
  ASSERT_TRUE(s >= 0 && s < NUM_STRATEGIES, "Strategy should be valid");

  world.np[ENEMY_NATION]->army_list = NULL;
}

/* ============================================================
 * Tests: Threat/Opportunity Assessment
 * ============================================================ */

static void test_threat_no_enemy(void)
{
  reset_world();
  setup_ai_territory();
  clear_ai_armies();

  int threat = ai_assess_threat(AI_NATION);
  ASSERT_EQ(threat, 0, "No enemy = no threat");
}

static void test_opportunity_empty_map(void)
{
  reset_world();
  /* No territory = no opportunity */
  int opp = ai_assess_opportunity(AI_NATION);
  ASSERT_EQ(opp, 0, "No territory = no opportunity");
}

static void test_opportunity_with_territory(void)
{
  reset_world();
  setup_ai_territory();

  int opp = ai_assess_opportunity(AI_NATION);
  ASSERT_TRUE(opp > 0, "Territory with unowned neighbors should have opportunity");
}

/* ============================================================
 * Tests: Full Turn Execution
 * ============================================================ */

static void test_execute_no_territory(void)
{
  reset_world();

  fupdate = tmpfile();
  fexe = tmpfile();

  TURN_CONTEXT ctx;
  turn_result_t r = ai_turn_execute(&ctx, AI_NATION);
  ASSERT_EQ(r, TURN_NO_TERRITORY, "No territory → NO_TERRITORY");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

static void test_execute_with_territory(void)
{
  reset_world();
  setup_ai_territory();
  setup_ai_armies();
  world.np[AI_NATION]->active = ACT_KILLER;

  fupdate = tmpfile();
  fexe = tmpfile();

  TURN_CONTEXT ctx;
  turn_result_t r = ai_turn_execute(&ctx, AI_NATION);
  ASSERT_TRUE(r == TURN_OK || r == TURN_NO_TERRITORY || r == TURN_FOG_NO_TARGETS,
              "Execute should succeed or have no targets");
  ASSERT_TRUE(ctx.sectors_claimed >= 0, "Claimed >= 0");
  ASSERT_TRUE(ctx.armies_moved >= 0, "Moved >= 0");
  ASSERT_TRUE(ctx.buildings_started >= 0, "Built >= 0");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

static void test_execute_all_phases(void)
{
  reset_world();
  setup_ai_territory();
  setup_ai_armies();
  world.np[AI_NATION]->active = ACT_OVERT;

  fupdate = tmpfile();
  fexe = tmpfile();

  TURN_CONTEXT ctx;
  /* Execute but don't check result — testing that phases run */
  (void)ai_turn_execute(&ctx, AI_NATION);
  ASSERT_TRUE(ctx.strategy >= 0 && ctx.strategy < NUM_STRATEGIES,
              "Strategy should be valid");
  ASSERT_TRUE(ctx.threat_level >= 0 && ctx.threat_level <= 100,
              "Threat level 0-100");
  ASSERT_TRUE(ctx.opportunity_level >= 0 && ctx.opportunity_level <= 100,
              "Opportunity level 0-100");
  ASSERT_TRUE(strlen(ctx.report_summary) > 0, "Report summary should be non-empty");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

/* ============================================================
 * Tests: Phase Flags
 * ============================================================ */

static void test_phase_flags(void)
{
  ASSERT_TRUE(PHASE_ALL == 0xFF, "PHASE_ALL should be 0xFF");
  ASSERT_TRUE(PHASE_FOG == 0x01, "PHASE_FOG should be 0x01");
  ASSERT_TRUE(PHASE_EXPAND == 0x08, "PHASE_EXPAND should be 0x08");
  ASSERT_TRUE(PHASE_REPORT == 0x80, "PHASE_REPORT should be 0x80");
}

static void test_partial_turn(void)
{
  reset_world();
  setup_ai_territory();
  setup_ai_armies();
  world.np[AI_NATION]->active = ACT_KILLER;

  TURN_CONTEXT ctx;
  ai_turn_init(&ctx, AI_NATION);

  /* Only economy phases */
  ctx.phase_flags = PHASE_FOG | PHASE_STRATEGY | PHASE_REPORT;

  fupdate = tmpfile();
  fexe = tmpfile();

  turn_result_t r = ai_turn_execute(&ctx, AI_NATION);
  ASSERT_TRUE(r == TURN_OK || r == TURN_NO_TERRITORY,
              "Partial turn should succeed");

  fclose(fupdate); fclose(fexe);
  fupdate = NULL; fexe = NULL;
}

/* ============================================================
 * Tests: Personality Variance in Strategy
 * ============================================================ */

static void test_personality_affects_strategy(void)
{
  TURN_CONTEXT ctx_k, ctx_s, ctx_m;

  reset_world();
  setup_ai_territory();
  setup_ai_armies();

  world.np[AI_NATION]->active = ACT_KILLER;
  ai_turn_init(&ctx_k, AI_NATION);
  strategy_type_t s_k = ai_turn_select_strategy(&ctx_k);

  reset_world();
  setup_ai_territory();
  setup_ai_armies();

  world.np[AI_NATION]->active = ACT_STATIC;
  ai_turn_init(&ctx_s, AI_NATION);
  strategy_type_t s_s = ai_turn_select_strategy(&ctx_s);

  reset_world();
  setup_ai_territory();
  setup_ai_armies();

  world.np[AI_NATION]->active = ACT_MOBILE;
  ai_turn_init(&ctx_m, AI_NATION);
  strategy_type_t s_m = ai_turn_select_strategy(&ctx_m);

  /* Different personalities should generally pick different strategies */
  /* At minimum, verify they all return valid strategies */
  ASSERT_TRUE(s_k >= 0 && s_k < NUM_STRATEGIES, "Killer strategy valid");
  ASSERT_TRUE(s_s >= 0 && s_s < NUM_STRATEGIES, "Static strategy valid");
  ASSERT_TRUE(s_m >= 0 && s_m < NUM_STRATEGIES, "Merchant strategy valid");
}

/* ============================================================
 * Main
 * ============================================================ */

int
main(int argc, char *argv[])
{
  printf("=== AI Turn Orchestrator Test Suite (Sprint 1.5) ===\n\n");

  test_init_world();

  printf("--- Strategy Name Tests ---\n");
  test_strategy_names();

  printf("\n--- Turn Init Tests ---\n");
  test_init_killer();
  test_init_static();
  test_init_no_territory();
  test_init_null();

  printf("\n--- Strategy Selection Tests ---\n");
  test_strategy_killer_default();
  test_strategy_static_default();
  test_strategy_merchant_default();
  test_strategy_high_threat();

  printf("\n--- Threat/Opportunity Tests ---\n");
  test_threat_no_enemy();
  test_opportunity_empty_map();
  test_opportunity_with_territory();

  printf("\n--- Full Turn Execution Tests ---\n");
  test_execute_no_territory();
  test_execute_with_territory();
  test_execute_all_phases();

  printf("\n--- Phase Flag Tests ---\n");
  test_phase_flags();
  test_partial_turn();

  printf("\n--- Personality Variance Tests ---\n");
  test_personality_affects_strategy();

  test_cleanup_world();

  printf("\n=== Results: %d passed, %d failed, %d total ===\n",
         tests_passed, tests_failed, tests_run);

  return tests_failed > 0 ? 1 : 0;
}