// SPDX-License-Identifier: GPL-3.0-or-later
/* fog_of_war.c - AI fog of war implementation */
/*
 * Conquer Reborn - Honest AI
 * Sprint 0: Warlord proof of concept
 *
 * Each AI nation maintains its own visibility map derived from
 * unit positions, city locations, and sector ownership. No cheats.
 *
 * Visibility rules:
 * - A nation can see sectors it owns
 * - A nation can see sectors adjacent to its units
 * - A nation can see sectors adjacent to its cities
 * - Flight units see further (range 2)
 * - Previously seen sectors are "remembered" but info goes stale
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ai/ai_standalone_types.h"
#include "ai/fog_of_war.h"

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

/* Convert (x,y) to linear index. Returns -1 if out of bounds. */
static int
fog_index(FOW_PTR fog, int x, int y)
{
  if (!fog || !fog->visible) return -1;
  if (x < 0 || x >= fog->map_width || y < 0 || y >= fog->map_height)
    return -1;
  return y * fog->map_width + x;
}

/* ------------------------------------------------------------------ */
/* Initialize fog state                                                */
/* ------------------------------------------------------------------ */

int
fog_init(FOW_PTR fog, ntntype nation_id, int map_width, int map_height)
{
  if (!fog || map_width <= 0 || map_height <= 0) return -1;

  int total_sectors = map_width * map_height;

  memset(fog, 0, sizeof(FOW_STRUCT));
  fog->nation_id = nation_id;
  fog->map_width = map_width;
  fog->map_height = map_height;
  fog->decay_rate = 0.15;	/* default: info decays at 15% per turn */
  fog->stale_threshold = 0.3;	/* below 0.3 freshness = stale */

  /* Allocate visibility bitmap */
  fog->visible = (char *)calloc(total_sectors, sizeof(char));
  if (!fog->visible) goto fail;

  /* Allocate known-sectors bitmap */
  fog->known = (char *)calloc(total_sectors, sizeof(char));
  if (!fog->known) goto fail;

  /* Allocate remembered state array */
  fog->remembered = (VIS_SECTOR_PTR)calloc(total_sectors, sizeof(VIS_SECTOR_STRUCT));
  if (!fog->remembered) goto fail;

  /* Initialize remembered state: all sectors unseen */
  for (int i = 0; i < total_sectors; i++) {
    fog->remembered[i].last_seen_turn = -1; /* never seen */
    fog->remembered[i].owner_when_seen = -1;
    fog->remembered[i].people_when_seen = 0;
    fog->remembered[i].designation_when_seen = -1;
    fog->remembered[i].efficiency_when_seen = -1;
    fog->remembered[i].changed = 0;
  }

  fog->visible_count = 0;
  fog->known_count = 0;

  return 0;

fail:
  free(fog->visible);
  free(fog->known);
  free(fog->remembered);
  fog->visible = NULL;
  fog->known = NULL;
  fog->remembered = NULL;
  return -1;
}

/* ------------------------------------------------------------------ */
/* Free fog state                                                      */
/* ------------------------------------------------------------------ */

void
fog_free(FOW_PTR fog)
{
  if (!fog) return;
  free(fog->visible);
  free(fog->known);
  free(fog->remembered);
  fog->visible = NULL;
  fog->known = NULL;
  fog->remembered = NULL;
  fog->visible_count = 0;
  fog->known_count = 0;
}

/* ------------------------------------------------------------------ */
/* Recalculate visibility from world state                              */
/* ------------------------------------------------------------------ */

int
fog_update(FOW_PTR fog, ntntype nation_id)
{
  if (!fog || !fog->visible) return -1;

  int total_sectors = fog->map_width * fog->map_height;
  int newly_visible = 0;

  /* Clear current visibility */
  memset(fog->visible, 0, total_sectors * sizeof(char));
  fog->visible_count = 0;

  /* We need to iterate over the world's nation, army, and city data.
   * For Sprint 0, we use the global world/ntn_ptr access pattern
   * from the Conquer codebase. The integration layer (task 0.4)
   * will wire this into cpu_update().
   *
   * For now, fog_update uses a callback pattern: the caller
   * provides visibility data through fog_mark_visible() calls
   * after fog_update() clears the map.
   *
   * This allows unit testing without the full Conquer world state.
   */

  return newly_visible;
}

/* ------------------------------------------------------------------ */
/* Mark a sector as visible (called by integration layer)              */
/* ------------------------------------------------------------------ */

/* Internal: mark a single sector visible */
static int
fog_mark_visible_internal(FOW_PTR fog, int x, int y, int *newly_visible)
{
  int idx = fog_index(fog, x, y);
  if (idx < 0) return -1;

  if (!fog->visible[idx]) {
    fog->visible[idx] = 1;
    fog->visible_count++;
    if (!fog->known[idx]) {
      fog->known[idx] = 1;
      fog->known_count++;
    }
    if (newly_visible) (*newly_visible)++;
  }

  return 0;
}

/* Public: mark a sector and its neighbors as visible within range */
int
fog_mark_visible_range(FOW_PTR fog, int cx, int cy, int range)
{
  if (!fog) return -1;

  int newly_visible = 0;

  /* Mark all sectors within 'range' of (cx, cy) */
  for (int dy = -range; dy <= range; dy++) {
    for (int dx = -range; dx <= range; dx++) {
      /* Chebyshev distance (square radius) for ground units,
       * or could use Manhattan for different vision model */
      int x = cx + dx;
      int y = cy + dy;
      fog_mark_visible_internal(fog, x, y, &newly_visible);
    }
  }

  return newly_visible;
}

/* Public: mark a single sector as visible */
int
fog_mark_visible(FOW_PTR fog, int x, int y)
{
  return fog_mark_visible_range(fog, x, y, 0);
}

/* ------------------------------------------------------------------ */
/* Observe: update remembered state for visible sectors                 */
/* ------------------------------------------------------------------ */

/* In the full integration, this reads SCT_STRUCT from the world map.
 * For Sprint 0 testing, we provide fog_observe_sector() to set
 * remembered state directly.
 */
int
fog_observe(FOW_PTR fog, ntntype nation_id)
{
  if (!fog || !fog->visible || !fog->remembered) return -1;

  int updated = 0;
  int total_sectors = fog->map_width * fog->map_height;

  /* For sectors currently visible, update remembered state.
   * In the integration layer, this reads from the actual world map.
   * For Sprint 0, the test harness will call fog_observe_sector()
   * directly.
   */
  for (int i = 0; i < total_sectors; i++) {
    if (fog->visible[i]) {
      /* Mark as needing update — integration layer fills in details */
      fog->remembered[i].changed = 1;
      updated++;
    }
  }

  return updated;
}

/* Set remembered state for a specific visible sector.
 * Used by integration layer and tests.
 */
int
fog_observe_sector(FOW_PTR fog, int x, int y, int current_turn,
                   ntntype owner, long people,
                   short designation, short efficiency)
{
  int idx = fog_index(fog, x, y);
  if (idx < 0) return -1;

  /* Can only observe sectors we can currently see */
  if (!fog->visible[idx]) return -1;

  VIS_SECTOR_PTR vis = &fog->remembered[idx];
  vis->last_seen_turn = current_turn;
  vis->owner_when_seen = owner;
  vis->people_when_seen = people;
  vis->designation_when_seen = designation;
  vis->efficiency_when_seen = efficiency;
  vis->changed = 1;

  return 0;
}

/* ------------------------------------------------------------------ */
/* Query functions                                                      */
/* ------------------------------------------------------------------ */

int
fog_can_see(FOW_PTR fog, int x, int y)
{
  int idx = fog_index(fog, x, y);
  if (idx < 0) return 0;
  return fog->visible[idx] ? 1 : 0;
}

int
fog_has_known(FOW_PTR fog, int x, int y)
{
  int idx = fog_index(fog, x, y);
  if (idx < 0) return 0;
  return fog->known[idx] ? 1 : 0;
}

double
fog_freshness(FOW_PTR fog, int x, int y, int current_turn)
{
  int idx = fog_index(fog, x, y);
  if (idx < 0) return 0.0;

  VIS_SECTOR_PTR vis = &fog->remembered[idx];

  /* Never seen = no freshness */
  if (vis->last_seen_turn < 0) return 0.0;

  /* Seen this turn = fresh */
  if (vis->last_seen_turn >= current_turn) return 1.0;

  /* Decay formula: 1.0 / (1.0 + turns_since_seen * decay_rate) */
  int turns_since = current_turn - vis->last_seen_turn;
  return 1.0 / (1.0 + (double)turns_since * fog->decay_rate);
}

VIS_SECTOR_PTR
fog_last_known(FOW_PTR fog, int x, int y)
{
  int idx = fog_index(fog, x, y);
  if (idx < 0) return NULL;

  /* Only return state for sectors we've ever seen */
  if (!fog->known[idx]) return NULL;

  return &fog->remembered[idx];
}

void
fog_mark_read(FOW_PTR fog, int x, int y)
{
  int idx = fog_index(fog, x, y);
  if (idx < 0) return;
  fog->remembered[idx].changed = 0;
}

/* ------------------------------------------------------------------ */
/* Reset                                                               */
/* ------------------------------------------------------------------ */

void
fog_reset(FOW_PTR fog, int current_turn)
{
  if (!fog) return;

  int total_sectors = fog->map_width * fog->map_height;

  /* Clear visibility */
  if (fog->visible) memset(fog->visible, 0, total_sectors * sizeof(char));
  fog->visible_count = 0;

  /* Clear known and remembered state */
  if (fog->known) memset(fog->known, 0, total_sectors * sizeof(char));
  fog->known_count = 0;

  /* Reset remembered sectors */
  if (fog->remembered) {
    for (int i = 0; i < total_sectors; i++) {
      fog->remembered[i].last_seen_turn = -1;
      fog->remembered[i].owner_when_seen = -1;
      fog->remembered[i].people_when_seen = 0;
      fog->remembered[i].designation_when_seen = -1;
      fog->remembered[i].efficiency_when_seen = -1;
      fog->remembered[i].changed = 0;
    }
  }
}

/* ------------------------------------------------------------------ */
/* Debug dump                                                          */
/* ------------------------------------------------------------------ */

void
fog_dump(FOW_PTR fog, int current_turn)
{
  if (!fog) {
    fprintf(stderr, "fog_dump: NULL fog state\n");
    return;
  }

  fprintf(stderr, "=== Fog of War: Nation %d ===\n", fog->nation_id);
  fprintf(stderr, "  Map: %dx%d (%d sectors)\n",
          fog->map_width, fog->map_height,
          fog->map_width * fog->map_height);
  fprintf(stderr, "  Visible: %d, Known: %d\n",
          fog->visible_count, fog->known_count);
  fprintf(stderr, "  Decay rate: %.2f, Stale threshold: %.2f\n",
          fog->decay_rate, fog->stale_threshold);

  /* Show first 10 visible sectors with freshness */
  int shown = 0;
  for (int y = 0; y < fog->map_height && shown < 10; y++) {
    for (int x = 0; x < fog->map_width && shown < 10; x++) {
      int idx = fog_index(fog, x, y);
      if (idx < 0) continue;
      if (fog->visible[idx] || fog->known[idx]) {
        double fresh = fog_freshness(fog, x, y, current_turn);
        fprintf(stderr, "  [%d,%d] vis=%d known=%d fresh=%.3f owner=%d mil=%ld\n",
                x, y,
                fog->visible[idx],
                fog->known[idx],
                fresh,
                fog->remembered[idx].owner_when_seen,
                fog->remembered[idx].people_when_seen);
        shown++;
      }
    }
  }
}