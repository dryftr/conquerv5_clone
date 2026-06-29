// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * test_mock_globals.c - Mock game globals for standalone test compilation
 * Sprint 1 Task 1.4c
 *
 * Key insight: In Conquer, UNOWNED = 0. Nation IDs start at 1.
 * Nation 0 means "no owner." Our test AI uses nation_id = 1.
 *
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */

#include "dataA.h"
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
 * Mock map dimensions
 * ============================================================ */

#define MOCK_MAPX 20
#define MOCK_MAPY 20

/* ============================================================
 * Global variables expected by game code
 * ============================================================ */

struct s_world world;
SCT_STRUCT **sct = NULL;
NTN_PTR ntn_ptr = NULL;
NTN_PTR ntn_tptr = NULL;
int country = 1;  /* Nation 1 = our AI (0 = UNOWNED) */
int global_int = 0;
long global_long = 0;
int no_input = 0;
int owneruid = 0;
int adjust_made = 0;
int xcurs = 0, ycurs = 0, xoffset = 0, yoffset = 0;
int movemode = 0;
FILE *fnews = NULL;
FILE *fexe = NULL;
FILE *fm = NULL;
FILE *fupdate = NULL;
ARMY_PTR army_ptr = NULL;
ARMY_PTR army_tptr = NULL;
DESG_STRUCT maj_dinfo[MAJ_NUMBER];

/* Mock nation array */
#define MOCK_MAX_NATIONS 8
static NTN_STRUCT mock_ntn[MOCK_MAX_NATIONS];

/* ============================================================
 * Mock initialization
 * ============================================================ */

void
test_init_world(void)
{
  memset(&world, 0, sizeof(struct s_world));
  world.mapx = MOCK_MAPX - 1;
  world.mapy = MOCK_MAPY - 1;
  world.nations = MOCK_MAX_NATIONS;
  world.turn = 1;

  /* Allocate sct array */
  sct = (SCT_STRUCT **)malloc(MOCK_MAPX * sizeof(SCT_STRUCT *));
  for (int x = 0; x < MOCK_MAPX; x++) {
    sct[x] = (SCT_STRUCT *)malloc(MOCK_MAPY * sizeof(SCT_STRUCT));
    for (int y = 0; y < MOCK_MAPY; y++) {
      memset(&sct[x][y], 0, sizeof(SCT_STRUCT));
      sct[x][y].owner = UNOWNED;  /* 0 = unowned */
      sct[x][y].designation = MAJ_NONE;
      sct[x][y].efficiency = 50;
      sct[x][y].people = 0;
      sct[x][y].minerals = 0;
      sct[x][y].tradegood = 0;
      sct[x][y].altitude = 50;
      sct[x][y].vegetation = 50;
    }
  }

  /* Initialize nations — nation 1 = AI player, nation 2 = enemy */
  memset(mock_ntn, 0, sizeof(mock_ntn));
  for (int i = 0; i < MOCK_MAX_NATIONS; i++) {
    mock_ntn[i].active = INACTIVE;
    mock_ntn[i].leftedge = 0;
    mock_ntn[i].rightedge = MOCK_MAPX - 1;
    mock_ntn[i].topedge = 0;
    mock_ntn[i].bottomedge = MOCK_MAPY - 1;
    mock_ntn[i].army_list = NULL;
    world.np[i] = &mock_ntn[i];
  }

  /* Nation 1 = our test AI (Killer by default) */
  mock_ntn[1].active = ACT_KILLER;
  ntn_ptr = &mock_ntn[1];
  country = 1;

  /* Initialize maj_dinfo with basic designation info */
  memset(maj_dinfo, 0, sizeof(maj_dinfo));
  maj_dinfo[MAJ_NONE].name = strdup("none");
  maj_dinfo[MAJ_FARM].name = strdup("farm");
  maj_dinfo[MAJ_METALMINE].name = strdup("mine");
  maj_dinfo[MAJ_TOWN].name = strdup("town");
  maj_dinfo[MAJ_STOCKADE].name = strdup("stockade");
  for (int i = 0; i < MAJ_NUMBER; i++) {
    if (maj_dinfo[i].name == NULL) {
      char buf[32];
      snprintf(buf, sizeof(buf), "desg_%d", i);
      maj_dinfo[i].name = strdup(buf);
    }
  }
}

void
test_cleanup_world(void)
{
  if (sct) {
    for (int x = 0; x < MOCK_MAPX; x++) {
      free(sct[x]);
    }
    free(sct);
    sct = NULL;
  }
}

/* ============================================================
 * Stub functions referenced by game headers but not needed for tests
 * ============================================================ */

int
npc_movearmy(int x, int y)
{
  (void)x;
  (void)y;
  return 1;
}

int
attract_val(int x, int y)
{
  int val = 0;
  if (!XY_ONMAP(x, y)) return 0;
  val += sct[x][y].people / 10;
  val += sct[x][y].minerals * 2;
  val += sct[x][y].tradegood * 5;
  val += sct[x][y].efficiency / 10;
  return val;
}

/* Stub for rove_army — legacy roving logic from npcA.c */
void
rove_army(void)
{
  /* In real game, this does legacy army roving.
   * For tests, just count it as a moved army. */
  if (army_ptr != NULL) {
    /* Simulate roving by marking the army as moved */
  }
}