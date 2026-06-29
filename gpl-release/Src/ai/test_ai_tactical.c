/* test_ai_tactical.c — Standalone tests for the tactical AI module
 * Sprint 2 Task 2.2: Combat engagement, retreat, garrison, reinforcement
 *
 * Pure logic tests: personality thresholds, ordering, allocation.
 * Game-integration tests compile with the full build system.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * Inline definitions (must match ai_tactical.h)
 * ============================================================ */

/* Personality type constants (from activeX.h) */
#define ACT_STATIC    0
#define ACT_ENFORCE   2
#define ACT_OVERT     3
#define ACT_MOBILE    4
#define ACT_KILLER    1
#define ACT_GUERRILA  5  /* Merchant in our mapping */

/* Attack ratio thresholds */
#define TAC_ATTACK_WARLORD      100
#define TAC_ATTACK_PIONEER      120
#define TAC_ATTACK_STRATEGIST   115
#define TAC_ATTACK_MERCHANT     150
#define TAC_ATTACK_FORTRESS     200

/* Retreat ratio thresholds */
#define TAC_RETREAT_WARLORD      30
#define TAC_RETREAT_PIONEER      50
#define TAC_RETREAT_STRATEGIST   40
#define TAC_RETREAT_MERCHANT     65
#define TAC_RETREAT_FORTRESS     75

/* Garrison thresholds */
#define TAC_GARRISON_WARLORD     30
#define TAC_GARRISON_PIONEER     50
#define TAC_GARRISON_STRATEGIST  40
#define TAC_GARRISON_MERCHANT    60
#define TAC_GARRISON_FORTRESS    20

/* Tactical target types */
typedef enum {
    TAC_ATTACK, TAC_DEFEND, TAC_RETREAT, TAC_REINFORCE, TAC_SCOUT, TAC_GARRISON
} tactical_type_t;

typedef struct s_tactical_target {
    int x, y;
    tactical_type_t type;
    int priority;
    int estimated_enemy;
    int estimated_friendly;
    int confidence;
    struct s_tactical_target *next;
} TACTICAL_TARGET, *TACTICAL_TARGET_PTR;

typedef struct s_tactical_state {
    int nation_id;
    int turn_number;
    int attack_ratio;
    int retreat_ratio;
    int garrison_threshold;
    int aggression;
    int caution;
    int border_focus;
    int attacks_launched;
    int defenses_ordered;
    int retreats_ordered;
    int reinforcements_sent;
    int garrisons_placed;
} TACTICAL_STATE, *TACTICAL_STATE_PTR;

/* ============================================================
 * Mirrored init logic (must match ai_tactical.c)
 * ============================================================ */

static void
set_attack_thresholds(TACTICAL_STATE_PTR state, int personality_type)
{
    switch (personality_type) {
    case ACT_OVERT:
        state->attack_ratio = TAC_ATTACK_WARLORD;
        state->retreat_ratio = TAC_RETREAT_WARLORD;
        state->garrison_threshold = TAC_GARRISON_WARLORD;
        state->aggression = 80;
        state->caution = 20;
        state->border_focus = 30;
        break;
    case ACT_MOBILE:
        state->attack_ratio = TAC_ATTACK_PIONEER;
        state->retreat_ratio = TAC_RETREAT_PIONEER;
        state->garrison_threshold = TAC_GARRISON_PIONEER;
        state->aggression = 60;
        state->caution = 40;
        state->border_focus = 50;
        break;
    case ACT_KILLER:
        state->attack_ratio = TAC_ATTACK_STRATEGIST;
        state->retreat_ratio = TAC_RETREAT_STRATEGIST;
        state->garrison_threshold = TAC_GARRISON_STRATEGIST;
        state->aggression = 70;
        state->caution = 30;
        state->border_focus = 40;
        break;
    case ACT_GUERRILA:
        state->attack_ratio = TAC_ATTACK_MERCHANT;
        state->retreat_ratio = TAC_RETREAT_MERCHANT;
        state->garrison_threshold = TAC_GARRISON_MERCHANT;
        state->aggression = 30;
        state->caution = 60;
        state->border_focus = 40;
        break;
    case ACT_ENFORCE:
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

/* Override personality for testing */
static int test_personality = ACT_OVERT;

static int
ai_tactical_init(TACTICAL_STATE_PTR state, int nation_id)
{
    if (state == NULL) return -1;
    memset(state, 0, sizeof(TACTICAL_STATE));
    state->nation_id = nation_id;
    set_attack_thresholds(state, test_personality);
    return 0;
}

static void
ai_tactical_free_targets(TACTICAL_TARGET_PTR list)
{
    while (list != NULL) {
        TACTICAL_TARGET_PTR next = list->next;
        free(list);
        list = next;
    }
}

/* ============================================================
 * Tests
 * ============================================================ */

static void test_init_warlord(void)
{
    TACTICAL_STATE state;
    test_personality = ACT_OVERT;
    int rc = ai_tactical_init(&state, 1);
    assert(rc == 0);
    assert(state.attack_ratio == TAC_ATTACK_WARLORD);
    assert(state.retreat_ratio == TAC_RETREAT_WARLORD);
    assert(state.garrison_threshold == TAC_GARRISON_WARLORD);
    assert(state.aggression == 80);
    assert(state.caution == 20);
    assert(state.border_focus == 30);
    printf("  TEST 1: Warlord init ................. PASS\n");
}

static void test_init_fortress(void)
{
    TACTICAL_STATE state;
    test_personality = ACT_ENFORCE;
    int rc = ai_tactical_init(&state, 2);
    assert(rc == 0);
    assert(state.attack_ratio == TAC_ATTACK_FORTRESS);
    assert(state.retreat_ratio == TAC_RETREAT_FORTRESS);
    assert(state.garrison_threshold == TAC_GARRISON_FORTRESS);
    assert(state.aggression == 20);
    assert(state.caution == 70);
    assert(state.border_focus == 80);
    printf("  TEST 2: Fortress init ................. PASS\n");
}

static void test_init_pioneer(void)
{
    TACTICAL_STATE state;
    test_personality = ACT_MOBILE;
    int rc = ai_tactical_init(&state, 3);
    assert(rc == 0);
    assert(state.attack_ratio == TAC_ATTACK_PIONEER);
    assert(state.retreat_ratio == TAC_RETREAT_PIONEER);
    printf("  TEST 3: Pioneer init .................. PASS\n");
}

static void test_init_merchant(void)
{
    TACTICAL_STATE state;
    test_personality = ACT_GUERRILA;
    int rc = ai_tactical_init(&state, 4);
    assert(rc == 0);
    assert(state.attack_ratio == TAC_ATTACK_MERCHANT);
    assert(state.retreat_ratio == TAC_RETREAT_MERCHANT);
    assert(state.aggression == 30);
    assert(state.caution == 60);
    printf("  TEST 4: Merchant init ................. PASS\n");
}

static void test_init_strategist(void)
{
    TACTICAL_STATE state;
    test_personality = ACT_KILLER;
    int rc = ai_tactical_init(&state, 5);
    assert(rc == 0);
    assert(state.attack_ratio == TAC_ATTACK_STRATEGIST);
    assert(state.retreat_ratio == TAC_RETREAT_STRATEGIST);
    printf("  TEST 5: Strategist init ................ PASS\n");
}

static void test_init_null(void)
{
    int rc = ai_tactical_init(NULL, 1);
    assert(rc == -1);
    printf("  TEST 6: Null state returns -1 ......... PASS\n");
}

static void test_attack_ordering(void)
{
    assert(TAC_ATTACK_WARLORD < TAC_ATTACK_STRATEGIST);
    assert(TAC_ATTACK_STRATEGIST < TAC_ATTACK_PIONEER);
    assert(TAC_ATTACK_PIONEER < TAC_ATTACK_MERCHANT);
    assert(TAC_ATTACK_MERCHANT < TAC_ATTACK_FORTRESS);
    printf("  TEST 7: Attack threshold ordering ..... PASS\n");
}

static void test_retreat_ordering(void)
{
    assert(TAC_RETREAT_WARLORD < TAC_RETREAT_STRATEGIST);
    assert(TAC_RETREAT_STRATEGIST < TAC_RETREAT_PIONEER);
    assert(TAC_RETREAT_PIONEER < TAC_RETREAT_MERCHANT);
    assert(TAC_RETREAT_MERCHANT < TAC_RETREAT_FORTRESS);
    printf("  TEST 8: Retreat threshold ordering .... PASS\n");
}

static void test_garrison_ordering(void)
{
    assert(TAC_GARRISON_FORTRESS < TAC_GARRISON_WARLORD);
    assert(TAC_GARRISON_WARLORD < TAC_GARRISON_STRATEGIST);
    assert(TAC_GARRISON_STRATEGIST < TAC_GARRISON_PIONEER);
    assert(TAC_GARRISON_PIONEER < TAC_GARRISON_MERCHANT);
    printf("  TEST 9: Garrison threshold ordering ... PASS\n");
}

static void test_warlord_aggressive(void)
{
    TACTICAL_STATE state;
    test_personality = ACT_OVERT;
    ai_tactical_init(&state, 1);
    assert(state.aggression > state.caution);
    assert(state.attack_ratio <= 110);
    assert(state.retreat_ratio <= 40);
    printf("  TEST 10: Warlord personality traits .... PASS\n");
}

static void test_fortress_defensive(void)
{
    TACTICAL_STATE state;
    test_personality = ACT_ENFORCE;
    ai_tactical_init(&state, 5);
    assert(state.caution > state.aggression);
    assert(state.border_focus > 70);
    assert(state.attack_ratio >= 200);
    assert(state.retreat_ratio >= 70);
    printf("  TEST 11: Fortress personality traits ... PASS\n");
}

static void test_merchant_cautious(void)
{
    TACTICAL_STATE state;
    test_personality = ACT_GUERRILA;
    ai_tactical_init(&state, 4);
    assert(state.attack_ratio >= 140);
    assert(state.retreat_ratio >= 60);
    assert(state.garrison_threshold >= 55);
    printf("  TEST 12: Merchant personality traits ... PASS\n");
}

static void test_target_types(void)
{
    assert(TAC_ATTACK != TAC_DEFEND);
    assert(TAC_ATTACK != TAC_RETREAT);
    assert(TAC_ATTACK != TAC_REINFORCE);
    assert(TAC_ATTACK != TAC_SCOUT);
    assert(TAC_ATTACK != TAC_GARRISON);
    assert(TAC_RETREAT != TAC_REINFORCE);
    assert(TAC_GARRISON != TAC_SCOUT);
    printf("  TEST 13: Target type distinct values ... PASS\n");
}

static void test_target_allocation(void)
{
    TACTICAL_TARGET_PTR tgt = (TACTICAL_TARGET_PTR)malloc(sizeof(TACTICAL_TARGET));
    assert(tgt != NULL);
    tgt->x = 5; tgt->y = 10;
    tgt->type = TAC_ATTACK;
    tgt->priority = 75;
    tgt->estimated_enemy = 200;
    tgt->estimated_friendly = 300;
    tgt->confidence = 80;
    tgt->next = NULL;
    assert(tgt->x == 5);
    assert(tgt->priority == 75);
    assert(tgt->estimated_enemy == 200);
    assert(tgt->confidence == 80);
    free(tgt);
    printf("  TEST 14: Target allocation/free ........ PASS\n");
}

static void test_result_init(void)
{
    TACTICAL_STATE state;
    memset(&state, 0, sizeof(TACTICAL_STATE));
    assert(state.attacks_launched == 0);
    assert(state.retreats_ordered == 0);
    assert(state.reinforcements_sent == 0);
    assert(state.garrisons_placed == 0);
    printf("  TEST 15: State tracking zero-init ..... PASS\n");
}

static void test_free_targets(void)
{
    TACTICAL_TARGET_PTR head = NULL;
    for (int i = 0; i < 5; i++) {
        TACTICAL_TARGET_PTR tgt = (TACTICAL_TARGET_PTR)malloc(sizeof(TACTICAL_TARGET));
        assert(tgt != NULL);
        tgt->x = i; tgt->y = i;
        tgt->type = TAC_ATTACK;
        tgt->priority = i * 10;
        tgt->next = head;
        head = tgt;
    }
    ai_tactical_free_targets(head);
    printf("  TEST 16: Free target list (5 nodes) ... PASS\n");
}

static void test_free_null(void)
{
    ai_tactical_free_targets(NULL);
    printf("  TEST 17: Free NULL targets ............ PASS\n");
}

static void test_state_zero_tracking(void)
{
    TACTICAL_STATE state;
    ai_tactical_init(&state, 1);
    assert(state.attacks_launched == 0);
    assert(state.retreats_ordered == 0);
    assert(state.reinforcements_sent == 0);
    assert(state.garrisons_placed == 0);
    printf("  TEST 18: State tracking after init .... PASS\n");
}

int main(void)
{
    printf("=== Tactical AI Module Tests ===\n\n");
    test_init_warlord();
    test_init_fortress();
    test_init_pioneer();
    test_init_merchant();
    test_init_strategist();
    test_init_null();
    test_attack_ordering();
    test_retreat_ordering();
    test_garrison_ordering();
    test_warlord_aggressive();
    test_fortress_defensive();
    test_merchant_cautious();
    test_target_types();
    test_target_allocation();
    test_result_init();
    test_free_targets();
    test_free_null();
    test_state_zero_tracking();
    printf("\n=== All 18 Tactical AI tests PASSED ===\n");
    return 0;
}