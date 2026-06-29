// SPDX-License-Identifier: GPL-3.0-or-later
/* fog_of_war.h - AI fog of war data structures and API */
/*
 * Conquer Reborn - Honest AI
 *
 * Each AI nation only knows what it can see. This module tracks
 * per-nation visibility, remembered (possibly stale) intelligence,
 * and information decay over time.
 *
 * Sprint 0: Basic sector visibility from unit/building positions,
 * remembered state, and freshness calculation.
 */

#ifndef FOG_OF_WAR_H
#define FOG_OF_WAR_H

/* Nation ID type.
 * ntntype must be defined before including this header.
 * When building with Conquer: sysconf.h defines it as uns_char.
 * For standalone testing: include a typedef before this header.
 */

/* ------------------------------------------------------------------ */
/* Visibility State for a Single Sector                                */
/* ------------------------------------------------------------------ */

typedef struct s_vis_sector {
  int last_seen_turn;		/* turn when last observed		*/
  ntntype owner_when_seen;	/* nation who owned it when seen	*/
  long people_when_seen;	/* civilian population when seen	*/
  short designation_when_seen;	/* sector designation when seen	*/
  short efficiency_when_seen;	/* sector efficiency when seen		*/
  int changed;			/* 1 if info changed since last read	*/
} VIS_SECTOR_STRUCT, *VIS_SECTOR_PTR;

/* ------------------------------------------------------------------ */
/* Per-Nation Fog of War State                                         */
/* ------------------------------------------------------------------ */

typedef struct s_fog {
  ntntype nation_id;		/* which nation owns this fog state	*/
  int map_width;		/* width of the visibility map		*/
  int map_height;		/* height of the visibility map	*/

  /* Currently visible sectors (can see right now this turn) */
  char *visible;			/* bitmap: 1 = currently visible	*/

  /* Remembered state (last known info per sector, may be stale) */
  VIS_SECTOR_PTR remembered;	/* array [map_width * map_height]	*/

  /* Sectors ever seen (even if not currently visible) */
  char *known;			/* bitmap: 1 = has ever been seen	*/

  /* Counters */
  int visible_count;		/* sectors currently visible		*/
  int known_count;		/* sectors ever seen			*/

  /* Configuration */
  double decay_rate;		/* how fast info freshness drops per turn */
  double stale_threshold;	/* freshness below this = stale		*/
} FOW_STRUCT, *FOW_PTR;

/* ------------------------------------------------------------------ */
/* API Functions                                                       */
/* ------------------------------------------------------------------ */

/* Initialize fog state for a nation. Allocates maps based on world size.
 * Returns 0 on success, -1 on error.
 */
int fog_init(FOW_PTR fog, ntntype nation_id, int map_width, int map_height);

/* Free all memory associated with a fog state */
void fog_free(FOW_PTR fog);

/* Recalculate currently visible sectors from unit/building positions.
 * A sector is visible if a nation's unit or city is within range.
 * Must be called once per turn before any fog queries.
 * Returns the number of newly visible sectors, or -1 on error.
 */
int fog_update(FOW_PTR fog, ntntype nation_id);

/* Update remembered state for all currently visible sectors.
 * Copies current world data into remembered state for sectors
 * the nation can currently see.
 * Returns number of sectors updated, or -1 on error.
 */
int fog_observe(FOW_PTR fog, ntntype nation_id);

/* Check if a nation can currently see a sector.
 * Returns 1 if visible this turn, 0 if not.
 */
int fog_can_see(FOW_PTR fog, int x, int y);

/* Check if a nation has ever seen a sector.
 * Returns 1 if known, 0 if never seen.
 */
int fog_has_known(FOW_PTR fog, int x, int y);

/* Get information freshness for a sector (0.0 = unknown, 1.0 = fresh).
 * Formula: 1.0 / (1.0 + (current_turn - last_seen_turn) * decay_rate)
 * Returns 0.0 for sectors never seen.
 */
double fog_freshness(FOW_PTR fog, int x, int y, int current_turn);

/* Get the last known state of a sector.
 * Returns pointer to remembered state, or NULL if never seen.
 * The data may be stale — check fog_freshness() to know how stale.
 */
VIS_SECTOR_PTR fog_last_known(FOW_PTR fog, int x, int y);

/* Mark that a sector's information has been consumed/read.
 * Clears the 'changed' flag on the remembered state.
 */
void fog_mark_read(FOW_PTR fog, int x, int y);

/* Reset all fog state (for new game or nation death). */
void fog_reset(FOW_PTR fog, int current_turn);

/* Mark a sector and its neighbors as visible within range.
 * range=0 means just the sector, range=1 means adjacent, etc.
 * Returns number of newly visible sectors, or -1 on error.
 */
int fog_mark_visible_range(FOW_PTR fog, int cx, int cy, int range);

/* Mark a single sector as visible.
 * Returns number of newly visible sectors (0 or 1), or -1 on error.
 */
int fog_mark_visible(FOW_PTR fog, int x, int y);

/* Set remembered state for a specific visible sector.
 * Used by integration layer and tests.
 */
int fog_observe_sector(FOW_PTR fog, int x, int y, int current_turn,
                      ntntype owner, long people,
                      short designation, short efficiency);

/* Dump fog state to stderr for debugging */
void fog_dump(FOW_PTR fog, int current_turn);

#endif /* FOG_OF_WAR_H */