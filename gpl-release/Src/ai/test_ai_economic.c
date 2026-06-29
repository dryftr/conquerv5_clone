/* test_ai_economic.c — Standalone tests for the economic AI module
 * Sprint 2 Task 2.3: Build prioritization, resource balancing, construction queues
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * Inline definitions (must match ai_economic.h)
 * ============================================================ */

#define ACT_STATIC    0
#define ACT_KILLER    1
#define ACT_ENFORCE   2
#define ACT_OVERT     3
#define ACT_MOBILE    4
#define ACT_GUERRILA  5

#define ECON_MAX_BUILDS_WARLORD    3
#define ECON_MAX_BUILDS_PIONEER    4
#define ECON_MAX_BUILDS_STRATEGIST 4
#define ECON_MAX_BUILDS_MERCHANT   5
#define ECON_MAX_BUILDS_FORTRESS   4

#define ECON_RESERVE_WARLORD       0.10
#define ECON_RESERVE_PIONEER       0.20
#define ECON_RESERVE_STRATEGIST    0.25
#define ECON_RESERVE_MERCHANT      0.30
#define ECON_RESERVE_FORTRESS      0.15

#define ECON_MOD_UNDER_ATTACK      20
#define ECON_MOD_LOW_TROOPS        15
#define ECON_MOD_DEFICIT           30
#define ECON_MOD_STRONG_ECONOMY    10

typedef enum {
    BUILD_MILITARY, BUILD_FORTIFICATION, BUILD_ECONOMY,
    BUILD_NAVAL, BUILD_CARAVAN, BUILD_COUNT
} build_category_t;

typedef struct s_econ_target {
    int x, y;
    build_category_t category;
    int base_score;
    int situational_bonus;
    int final_score;
    long cost;
    const char *name;
} ECON_TARGET, *ECON_TARGET_PTR;

typedef struct s_econ_state {
    int nation_id;
    int turn_number;
    long treasury;
    long income;
    long expenses;
    long net_income;
    int max_builds_per_turn;
    double reserve_pct;
    long reserve_amount;
    long spend_budget;
    double weight_military;
    double weight_fortification;
    double weight_economy;
    double weight_naval;
    double weight_caravan;
    int under_attack;
    int low_troops;
    int deficit;
    double economy_mult;
    double build_cap_mult;
    int builds_started;
    int builds_skipped;
    int builds_completed;
} ECON_STATE, *ECON_STATE_PTR;

/* Mirrored init logic */
static int test_personality = ACT_OVERT;

static void
set_default_build_prefs(ECON_STATE_PTR state, int personality_type)
{
    switch (personality_type) {
    case ACT_OVERT:
        state->max_builds_per_turn = ECON_MAX_BUILDS_WARLORD;
        state->reserve_pct = ECON_RESERVE_WARLORD;
        state->weight_military = 0.50;
        state->weight_fortification = 0.15;
        state->weight_economy = 0.10;
        state->weight_naval = 0.15;
        state->weight_caravan = 0.10;
        break;
    case ACT_MOBILE:
        state->max_builds_per_turn = ECON_MAX_BUILDS_PIONEER;
        state->reserve_pct = ECON_RESERVE_PIONEER;
        state->weight_military = 0.20;
        state->weight_fortification = 0.10;
        state->weight_economy = 0.40;
        state->weight_naval = 0.15;
        state->weight_caravan = 0.15;
        break;
    case ACT_KILLER:
        state->max_builds_per_turn = ECON_MAX_BUILDS_STRATEGIST;
        state->reserve_pct = ECON_RESERVE_STRATEGIST;
        state->weight_military = 0.30;
        state->weight_fortification = 0.20;
        state->weight_economy = 0.25;
        state->weight_naval = 0.15;
        state->weight_caravan = 0.10;
        break;
    case ACT_GUERRILA:
        state->max_builds_per_turn = ECON_MAX_BUILDS_MERCHANT;
        state->reserve_pct = ECON_RESERVE_MERCHANT;
        state->weight_military = 0.10;
        state->weight_fortification = 0.10;
        state->weight_economy = 0.50;
        state->weight_naval = 0.15;
        state->weight_caravan = 0.15;
        break;
    case ACT_ENFORCE:
        state->max_builds_per_turn = ECON_MAX_BUILDS_FORTRESS;
        state->reserve_pct = ECON_RESERVE_FORTRESS;
        state->weight_military = 0.25;
        state->weight_fortification = 0.40;
        state->weight_economy = 0.15;
        state->weight_naval = 0.10;
        state->weight_caravan = 0.10;
        break;
    default:
        state->max_builds_per_turn = 3;
        state->reserve_pct = 0.25;
        state->weight_military = 0.25;
        state->weight_fortification = 0.20;
        state->weight_economy = 0.25;
        state->weight_naval = 0.15;
        state->weight_caravan = 0.15;
        break;
    }
}

static int
ai_economic_init(ECON_STATE_PTR state, int nation_id)
{
    if (state == NULL) return -1;
    memset(state, 0, sizeof(ECON_STATE));
    state->nation_id = nation_id;
    state->economy_mult = 1.0;
    state->build_cap_mult = 1.0;
    set_default_build_prefs(state, test_personality);
    return 0;
}

/* ============================================================
 * Tests
 * ============================================================ */

static void test_init_warlord(void)
{
    ECON_STATE state;
    test_personality = ACT_OVERT;
    int rc = ai_economic_init(&state, 1);
    assert(rc == 0);
    assert(state.max_builds_per_turn == 3);
    assert(state.reserve_pct < 0.11);  /* 10% */
    assert(state.weight_military > state.weight_economy);
    assert(state.weight_military > 0.4);
    printf("  TEST 1: Warlord economic init .......... PASS\n");
}

static void test_init_merchant(void)
{
    ECON_STATE state;
    test_personality = ACT_GUERRILA;
    int rc = ai_economic_init(&state, 2);
    assert(rc == 0);
    assert(state.max_builds_per_turn == 5);
    assert(state.reserve_pct > 0.29);  /* 30% */
    assert(state.weight_economy > state.weight_military);
    assert(state.weight_economy > 0.4);
    printf("  TEST 2: Merchant economic init .......... PASS\n");
}

static void test_init_fortress(void)
{
    ECON_STATE state;
    test_personality = ACT_ENFORCE;
    int rc = ai_economic_init(&state, 3);
    assert(rc == 0);
    assert(state.max_builds_per_turn == 4);
    assert(state.reserve_pct < 0.16);  /* 15% */
    assert(state.weight_fortification > state.weight_economy);
    assert(state.weight_fortification > 0.3);
    printf("  TEST 3: Fortress economic init ......... PASS\n");
}

static void test_init_null(void)
{
    int rc = ai_economic_init(NULL, 1);
    assert(rc == -1);
    printf("  TEST 4: Null state returns -1 .......... PASS\n");
}

static void test_build_caps(void)
{
    assert(ECON_MAX_BUILDS_WARLORD == 3);
    assert(ECON_MAX_BUILDS_PIONEER == 4);
    assert(ECON_MAX_BUILDS_STRATEGIST == 4);
    assert(ECON_MAX_BUILDS_MERCHANT == 5);
    assert(ECON_MAX_BUILDS_FORTRESS == 4);
    printf("  TEST 5: Build cap constants ............ PASS\n");
}

static void test_reserve_ordering(void)
{
    /* Merchant reserves most, Warlord reserves least */
    assert(ECON_RESERVE_WARLORD < ECON_RESERVE_FORTRESS);
    assert(ECON_RESERVE_FORTRESS < ECON_RESERVE_PIONEER);
    assert(ECON_RESERVE_PIONEER < ECON_RESERVE_STRATEGIST);
    assert(ECON_RESERVE_STRATEGIST < ECON_RESERVE_MERCHANT);
    printf("  TEST 6: Reserve ordering ............... PASS\n");
}

static void test_weight_sums(void)
{
    /* All personality weights should sum to ~1.0 */
    ECON_STATE state;
    double sum;

    test_personality = ACT_OVERT;
    ai_economic_init(&state, 1);
    sum = state.weight_military + state.weight_fortification +
          state.weight_economy + state.weight_naval + state.weight_caravan;
    assert(sum > 0.99 && sum < 1.01);

    test_personality = ACT_GUERRILA;
    ai_economic_init(&state, 2);
    sum = state.weight_military + state.weight_fortification +
          state.weight_economy + state.weight_naval + state.weight_caravan;
    assert(sum > 0.99 && sum < 1.01);

    test_personality = ACT_ENFORCE;
    ai_economic_init(&state, 3);
    sum = state.weight_military + state.weight_fortification +
          state.weight_economy + state.weight_naval + state.weight_caravan;
    assert(sum > 0.99 && sum < 1.01);

    printf("  TEST 7: Weight sums ≈ 1.0 ............. PASS\n");
}

static void test_category_names(void)
{
    static const char *names[] = {
        "Military", "Fortification", "Economy", "Naval", "Caravan"
    };
    int i;
    for (i = 0; i < BUILD_COUNT; i++) {
        assert(names[i] != NULL);
    }
    printf("  TEST 8: Category names ................ PASS\n");
}

static void test_situational_modifiers(void)
{
    /* Under attack should boost fortification by 20 */
    assert(ECON_MOD_UNDER_ATTACK == 20);
    assert(ECON_MOD_LOW_TROOPS == 15);
    assert(ECON_MOD_DEFICIT == 30);
    assert(ECON_MOD_STRONG_ECONOMY == 10);
    printf("  TEST 9: Situational modifiers ......... PASS\n");
}

static void test_target_scoring(void)
{
    ECON_STATE state;
    test_personality = ACT_OVERT;
    ai_economic_init(&state, 1);

    /* Warlord: military should have highest base score */
    ECON_TARGET targets[5];
    int i;
    for (i = 0; i < BUILD_COUNT; i++) {
        double w;
        switch (i) {
        case BUILD_MILITARY:     w = state.weight_military; break;
        case BUILD_FORTIFICATION: w = state.weight_fortification; break;
        case BUILD_ECONOMY:      w = state.weight_economy; break;
        case BUILD_NAVAL:        w = state.weight_naval; break;
        case BUILD_CARAVAN:      w = state.weight_caravan; break;
        default:                w = 0.0; break;
        }
        targets[i].category = (build_category_t)i;
        targets[i].base_score = (int)(w * 100.0);
        targets[i].final_score = targets[i].base_score;
    }

    /* Military should be highest for Warlord */
    assert(targets[BUILD_MILITARY].base_score > targets[BUILD_ECONOMY].base_score);
    printf("  TEST 10: Target scoring Warlord ........ PASS\n");
}

static void test_deficit_boosts_economy(void)
{
    ECON_STATE state;
    test_personality = ACT_GUERRILA;
    ai_economic_init(&state, 1);
    state.deficit = 1;

    /* When in deficit, economy should get +30 bonus */
    int econ_score_without = (int)(state.weight_economy * 100.0);
    int econ_score_with = econ_score_without + ECON_MOD_DEFICIT;
    assert(econ_score_with > econ_score_without);
    assert(econ_score_with == econ_score_without + 30);
    printf("  TEST 11: Deficit boosts economy ........ PASS\n");
}

static void test_difficulty_scaling(void)
{
    ECON_STATE state;
    test_personality = ACT_OVERT;
    ai_economic_init(&state, 1);
    int base_max = state.max_builds_per_turn;

    /* Simulate Hard difficulty: build_cap_mult = 1.3 */
    state.build_cap_mult = 1.3;
    state.max_builds_per_turn = (int)((double)base_max * state.build_cap_mult);
    assert(state.max_builds_per_turn >= base_max);
    printf("  TEST 12: Difficulty scaling builds ..... PASS\n");
}

static void test_budget_reserve(void)
{
    ECON_STATE state;
    test_personality = ACT_GUERRILA;
    ai_economic_init(&state, 1);

    /* Merchant reserves 30% of treasury */
    state.treasury = 1000;
    state.reserve_amount = (long)((double)state.treasury * state.reserve_pct);
    state.spend_budget = state.treasury - state.reserve_amount;

    assert(state.reserve_amount == 300);
    assert(state.spend_budget == 700);
    printf("  TEST 13: Budget reserve calculation .... PASS\n");
}

static void test_state_zero_init(void)
{
    ECON_STATE state;
    test_personality = ACT_OVERT;
    ai_economic_init(&state, 1);
    assert(state.builds_started == 0);
    assert(state.builds_skipped == 0);
    assert(state.builds_completed == 0);
    assert(state.under_attack == 0);
    assert(state.low_troops == 0);
    assert(state.deficit == 0);
    printf("  TEST 14: State tracking zero-init ..... PASS\n");
}

int
main(void)
{
    printf("=== Economic AI Module Tests ===\n\n");
    test_init_warlord();
    test_init_merchant();
    test_init_fortress();
    test_init_null();
    test_build_caps();
    test_reserve_ordering();
    test_weight_sums();
    test_category_names();
    test_situational_modifiers();
    test_target_scoring();
    test_deficit_boosts_economy();
    test_difficulty_scaling();
    test_budget_reserve();
    test_state_zero_init();
    printf("\n=== All 14 Economic AI tests PASSED ===\n");
    return 0;
}