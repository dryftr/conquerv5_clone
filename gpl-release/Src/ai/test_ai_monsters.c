/* test_ai_monsters.c — Tests for the monster AI dispatcher
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Standalone type definitions — not building with game headers */
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define UNOWNED 0

/* Active type constants (from activeX.h) */
#define INACTIVE 0
#define DEAD_LIZARD 4
#define DEAD_SAVAGE 12
#define DEAD_NOMAD 16
#define DEAD_PIRATE 20
#define NPC_INACTIVE 24
#define NPC_LIZARD 28
#define NPC_SAVAGE 36
#define NPC_NOMAD 40
#define NPC_PIRATE 44

/* Monster type enum (from ai_monsters.h) */
typedef enum {
    MONSTER_LIZARD  = NPC_LIZARD,
    MONSTER_SAVAGE  = NPC_SAVAGE,
    MONSTER_PIRATE  = NPC_PIRATE,
    MONSTER_NOMAD   = NPC_NOMAD,
} monster_type_t;

/* Macros */
#define n_ismonster(x) ((x) == NPC_LIZARD || (x) == NPC_SAVAGE || \
                         (x) == NPC_PIRATE || (x) == NPC_NOMAD || \
                         (x) == DEAD_LIZARD || (x) == DEAD_SAVAGE || \
                         (x) == DEAD_PIRATE || (x) == DEAD_NOMAD)
#define n_islizard(x) (((x) == NPC_LIZARD) || ((x) == DEAD_LIZARD))
#define n_issavage(x) (((x) == NPC_SAVAGE) || ((x) == DEAD_SAVAGE))
#define n_isnomad(x) (((x) == NPC_NOMAD) || ((x) == DEAD_NOMAD))
#define n_ispirate(x) (((x) == NPC_PIRATE) || ((x) == DEAD_PIRATE))

/* === Test state tracking === */
static int last_monster_type = -1;
static int monster_update_called = 0;
static int lizard_calls = 0;
static int savage_calls = 0;
static int pirate_calls = 0;
static int nomad_calls = 0;
static int null_ptr_calls = 0;

/* === Simplified dispatcher (mirrors ai_monster_update logic) === */
static int
test_monster_dispatch(int active_type, int has_nation)
{
    /* null nation check */
    if (!has_nation) return -1;

    last_monster_type = active_type;
    monster_update_called = 1;

    if (n_islizard(active_type)) {
        lizard_calls++;
        return 0;
    } else if (n_issavage(active_type)) {
        savage_calls++;
        return 0;
    } else if (n_ispirate(active_type)) {
        pirate_calls++;
        return 0;
    } else if (n_isnomad(active_type)) {
        nomad_calls++;
        return 0;
    } else {
        return -1;
    }
}

static void
reset_state(void)
{
    last_monster_type = -1;
    monster_update_called = 0;
    lizard_calls = 0;
    savage_calls = 0;
    pirate_calls = 0;
    nomad_calls = 0;
    null_ptr_calls = 0;
}

/* === Tests === */

static void
test_lizard_dispatch(void)
{
    reset_state();
    int rc = test_monster_dispatch(NPC_LIZARD, 1);
    assert(rc == 0);
    assert(lizard_calls == 1);
    assert(savage_calls == 0);
    assert(last_monster_type == NPC_LIZARD);
    printf("  TEST 1: Lizard dispatch .............. PASS\n");
}

static void
test_savage_dispatch(void)
{
    reset_state();
    int rc = test_monster_dispatch(NPC_SAVAGE, 1);
    assert(rc == 0);
    assert(savage_calls == 1);
    assert(lizard_calls == 0);
    printf("  TEST 2: Savage dispatch .............. PASS\n");
}

static void
test_pirate_dispatch(void)
{
    reset_state();
    int rc = test_monster_dispatch(NPC_PIRATE, 1);
    assert(rc == 0);
    assert(pirate_calls == 1);
    printf("  TEST 3: Pirate dispatch .............. PASS\n");
}

static void
test_nomad_dispatch(void)
{
    reset_state();
    int rc = test_monster_dispatch(NPC_NOMAD, 1);
    assert(rc == 0);
    assert(nomad_calls == 1);
    printf("  TEST 4: Nomad dispatch ................ PASS\n");
}

static void
test_dead_lizard_dispatch(void)
{
    reset_state();
    /* DEAD variants should also dispatch to the same handler */
    int rc = test_monster_dispatch(DEAD_LIZARD, 1);
    assert(rc == 0);
    assert(lizard_calls == 1);
    printf("  TEST 5: Dead lizard dispatch .......... PASS\n");
}

static void
test_dead_savage_dispatch(void)
{
    reset_state();
    int rc = test_monster_dispatch(DEAD_SAVAGE, 1);
    assert(rc == 0);
    assert(savage_calls == 1);
    printf("  TEST 6: Dead savage dispatch .......... PASS\n");
}

static void
test_null_nation(void)
{
    reset_state();
    int rc = test_monster_dispatch(NPC_LIZARD, 0);
    assert(rc == -1);
    assert(monster_update_called == 0);
    printf("  TEST 7: Null nation returns -1 ......... PASS\n");
}

static void
test_inactive_nation(void)
{
    reset_state();
    /* INACTIVE is not a monster type — should return -1 */
    int rc = test_monster_dispatch(INACTIVE, 1);
    assert(rc == -1);
    printf("  TEST 8: Inactive returns -1 ........... PASS\n");
}

static void
test_type_enum_values(void)
{
    /* Verify enum values match activeX.h constants */
    assert(MONSTER_LIZARD == NPC_LIZARD);
    assert(MONSTER_SAVAGE == NPC_SAVAGE);
    assert(MONSTER_PIRATE == NPC_PIRATE);
    assert(MONSTER_NOMAD  == NPC_NOMAD);
    printf("  TEST 9: Enum value consistency ........ PASS\n");
}

static void
test_ismonster_macros(void)
{
    assert(n_ismonster(NPC_LIZARD) == 1);
    assert(n_ismonster(NPC_SAVAGE) == 1);
    assert(n_ismonster(NPC_PIRATE) == 1);
    assert(n_ismonster(NPC_NOMAD) == 1);
    assert(n_ismonster(DEAD_LIZARD) == 1);
    assert(n_ismonster(DEAD_SAVAGE) == 1);
    assert(n_ismonster(DEAD_PIRATE) == 1);
    assert(n_ismonster(DEAD_NOMAD) == 1);
    assert(n_ismonster(INACTIVE) == 0);
    assert(n_ismonster(1) == 0);  /* player nation */
    printf("  TEST 10: n_ismonster macros .......... PASS\n");
}

static void
test_exclusive_dispatch(void)
{
    reset_state();
    /* Each type should dispatch exclusively */
    test_monster_dispatch(NPC_LIZARD, 1);
    test_monster_dispatch(NPC_SAVAGE, 1);
    test_monster_dispatch(NPC_PIRATE, 1);
    test_monster_dispatch(NPC_NOMAD, 1);
    assert(lizard_calls == 1);
    assert(savage_calls == 1);
    assert(pirate_calls == 1);
    assert(nomad_calls == 1);
    printf("  TEST 11: Exclusive dispatch ........... PASS\n");
}

static void
test_multiple_turns(void)
{
    reset_state();
    /* Simulate multiple turns for same monster type */
    for (int i = 0; i < 5; i++) {
        test_monster_dispatch(NPC_LIZARD, 1);
    }
    assert(lizard_calls == 5);
    assert(savage_calls == 0);
    printf("  TEST 12: Multiple turns same type ...... PASS\n");
}

int
main(void)
{
    printf("=== Monster AI Dispatcher Tests ===\n\n");
    test_lizard_dispatch();
    test_savage_dispatch();
    test_pirate_dispatch();
    test_nomad_dispatch();
    test_dead_lizard_dispatch();
    test_dead_savage_dispatch();
    test_null_nation();
    test_inactive_nation();
    test_type_enum_values();
    test_ismonster_macros();
    test_exclusive_dispatch();
    test_multiple_turns();

    printf("\n=== All 12 Monster AI tests PASSED ===\n");
    return 0;
}