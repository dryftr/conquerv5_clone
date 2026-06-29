// SPDX-License-Identifier: GPL-3.0-or-later
/* personality.h - AI personality data structures and loader */
/*
 * Conquer Reborn - Honest AI Personality System
 *
 * Each AI nation is driven by a personality that determines how it
 * perceives the world and makes decisions. No cheating - the AI
 * only knows what it can see through fog of war.
 *
 * Sprint 0: Warlord proof of concept. Five personalities planned.
 */

#ifndef PERSONALITY_H
#define PERSONALITY_H

/* Forward declaration - Diplotype is defined in dstatusX.h
 * We include it only when building with the full Conquer codebase.
 * For standalone testing, we define a minimal version. */
#ifndef DIPLOTYPE_DEFINED
#define DIPLOTYPE_DEFINED
  typedef enum diplotype Diplotype;
#endif

/* Conquer type definitions - match dataX.h when building in-tree */
#ifndef NAMELTH
#define NAMELTH	10
#endif
#ifndef BIGLTH
#define BIGLTH	500
#endif
#ifndef FILELTH
#define FILELTH	255
#endif

/* ------------------------------------------------------------------ */
/* Personality Archetypes                                              */
/* ------------------------------------------------------------------ */

typedef enum {
  PERSONALITY_WARLORD,	/* Aggressive expansion, military focus */
  PERSONALITY_FORTRESS,	/* Defensive stronghold, slow expansion */
  PERSONALITY_MERCHANT,	/* Economic growth, trade-oriented */
  PERSONALITY_PIONEER,	/* Rapid expansion, exploration focus */
  PERSONALITY_STRATEGIST,/* Balanced, adaptive long-term planner */
  PERSONALITY_COUNT	/* total number of personality types */
} PersonalityType;

/* ------------------------------------------------------------------ */
/* Priority Weights - how much each domain matters to this personality */
/* ------------------------------------------------------------------ */

typedef struct s_priority {
  double military;	/* troop building, combat actions	*/
  double economy;	/* resource production, trade		*/
  double expansion;	/* claiming new sectors		*/
  double defense;	/* fortifying, garrisoning		*/
  double diplomacy;	/* alliances, treaties, trade deals	*/
  double scouting;	/* exploration, information gathering	*/
  double research;	/* magic, technology advancement	*/
} PRIORITY_STRUCT, *PRIORITY_PTR;

/* ------------------------------------------------------------------ */
/* Build Preferences - what to construct when resources are available */
/* ------------------------------------------------------------------ */

typedef struct s_buildpref {
  double military_units;	/* preference for building armies	*/
  double fortifications;	/* preference for fortifying cities	*/
  double economic_buildings;	/* preference for production buildings	*/
  double naval_units;		/* preference for building ships	*/
  double caravans;		/* preference for building caravans	*/
} BUILDPREF_STRUCT, *BUILDPREF_PTR;

/* ------------------------------------------------------------------ */
/* Starting Resource Allocation - how to spend the first turn's budget */
/* ------------------------------------------------------------------ */

typedef struct s_startalloc {
  double military_pct;	/* % of budget for military		*/
  double economy_pct;	/* % of budget for economy		*/
  double expansion_pct;	/* % of budget for expansion		*/
  double defense_pct;	/* % of budget for defense		*/
  double scouting_pct;	/* % of budget for scouting		*/
} STARTALLOC_STRUCT, *STARTALLOC_PTR;

/* ------------------------------------------------------------------ */
/* Scout Behavior Parameters                                           */
/* ------------------------------------------------------------------ */

typedef struct s_scoutcfg {
  double scout_pct;		/* % of units allocated to scouting	*/
  int    scout_range;		/* how far scouts roam from borders	*/
  double explore_unknown_weight;	/* weight for unexplored sectors	*/
  double revisit_stale_weight;	/* weight for revisiting stale sectors	*/
  double border_priority_weight;	/* weight for border surveillance	*/
} SCOUTCFG_STRUCT, *SCOUTCFG_PTR;

/* ------------------------------------------------------------------ */
/* Diplomacy Tendency - how this personality approaches relations      */
/* ------------------------------------------------------------------ */

typedef struct s_diplomacy {
  double trust_initial;		/* starting trust level (0.0-1.0)	*/
  double trust_growth_rate;	/* how fast trust builds per turn	*/
  double trust_decay_rate;	/* how fast trust erodes per turn	*/
  double betrayal_threshold;	/* trust level below which betrayal risk */
  double alliance_willingness;	/* how eager to form alliances		*/
  double war_threshold;		/* provocation level needed to declare war */
  int default_stance;		/* starting diplomatic stance (Diplotype)	*/
} DIPLOMACY_STRUCT, *DIPLOMACY_PTR;

/* ------------------------------------------------------------------ */
/* Full Personality Structure                                          */
/* ------------------------------------------------------------------ */

typedef struct s_personality {
  PersonalityType type;		/* personality archetype		*/
  char name[NAMELTH+1];		/* display name				*/
  char description[BIGLTH];	/* flavor text for reports		*/

  /* Core weights - base priorities before adaptation */
  PRIORITY_STRUCT base_priority;	/* fixed personality weights	*/
  PRIORITY_STRUCT effective_priority;	/* after adaptation blending	*/

  /* Decision parameters */
  double risk_tolerance;	/* 0.0 (cautious) to 1.0 (reckless)	*/
  double reaction_speed;	/* 0.0 (stubborn) to 1.0 (reactive)	*/
  double adaptation_rate;	/* how fast effective_priority shifts	*/
  double patience;		/* turns willing to wait for long-term plans */

  /* Build and starting allocation */
  BUILDPREF_STRUCT build_pref;	/* construction preferences		*/
  STARTALLOC_STRUCT start_alloc;/* first-turn resource split		*/

  /* Scouting */
  SCOUTCFG_STRUCT scout_cfg;	/* how this personality scouts		*/

  /* Diplomacy */
  DIPLOMACY_STRUCT diplomacy;	/* how this personality negotiates	*/

  /* Combat preferences */
  double attack_preference;	/* vs defense when both viable (0.0-1.0)	*/
  double retreat_threshold;	/* strength ratio below which we retreat	*/
  double siege_patience;	/* turns willing to maintain a siege	*/

  /* Expansion */
  double expansion_aggression;	/* how far from borders to expand	*/
  int   claim_cap;                /* max sectors to claim/turn (2-8)  */
  double territory_focus;	/* 0.0 (compact) to 1.0 (sprawling)	*/

  /* Economic parameters (Sprint 2.3 — JSON-driven) */
  int    garrison_threshold;	/* min threat level to place garrison (0-100)	*/
  double reserve_pct;		/* fraction of treasury to reserve (0.0-1.0)	*/
  int    max_builds_per_turn;	/* max buildings per turn (1-5)			*/

  /* Loaded flag */
  int loaded;			/* 1 if personality data is valid	*/
} PERSONALITY_STRUCT, *PERSONALITY_PTR;

/* ------------------------------------------------------------------ */
/* API Functions                                                       */
/* ------------------------------------------------------------------ */

/* Load a personality from JSON file. Returns 0 on success, -1 on error.
 * Searches Auxil/personalities/<name>.json relative to game data dir.
 */
int personality_load(const char *name, PERSONALITY_PTR pers);

/* Get the effective priority for a personality, blending base weights
 * with situational weights using the adaptation formula:
 *   effective = base * (1 - reaction_speed) + situational * reaction_speed
 */
void personality_get_effective_weight(PERSONALITY_PTR pers,
                                     PRIORITY_PTR situational,
                                     PRIORITY_PTR result);

/* Get personality type from name string. Returns -1 if not found. */
PersonalityType personality_type_from_name(const char *name);

/* Get display name for a personality type. Returns NULL if invalid. */
const char *personality_name_from_type(PersonalityType type);

/* Validate a loaded personality struct. Returns 0 if valid, -1 if not. */
int personality_validate(PERSONALITY_PTR pers);

/* Dump personality to stderr for debugging */
void personality_dump(PERSONALITY_PTR pers);

/* ------------------------------------------------------------------ */
/* Difficulty Configuration                                              */
/* ------------------------------------------------------------------ */

/* Difficulty levels for AI behavior.
 * Multipliers are applied AFTER personality JSON values.
 * A Warlord on Easy is still aggressive, but less efficient.
 * A Merchant on Hard is still economy-focused, but sharper.
 */
typedef enum {
  DIFFICULTY_EASY = 0,		/* AI weaker: less efficient economy, worse combat ratios */
  DIFFICULTY_NORMAL = 1,	/* AI balanced: personality values as-is */
  DIFFICULTY_HARD = 2,		/* AI stronger: better economy, more aggressive combat */
  DIFFICULTY_BRUTAL = 3,	/* AI relentless: maximum efficiency */
  DIFFICULTY_COUNT		/* total difficulty levels */
} DifficultyLevel;

typedef struct s_difficulty_config {
  double economy_mult;		/* multiplier on AI economy efficiency (0.5-1.5) */
  double attack_mult;		/* multiplier on AI attack effectiveness (0.7-1.3) */
  double reserve_pct_mult;	/* multiplier on AI treasury reserve (0.5-2.0) */
  double build_cap_mult;		/* multiplier on max builds per turn (0.5-2.0) */
  double expansion_mult;		/* multiplier on AI claim cap per turn (0.5-2.0) */
  int    garrison_bonus;		/* extra garrison troops on Hard/Brutal (0-2) */
  double vision_bonus;		/* extra vision range (0-2 sectors) */
} DIFFICULTY_CONFIG, *DIFFICULTY_CONFIG_PTR;

/* Difficulty presets */
#define DIFF_EASY   { 0.6, 0.7, 0.5, 0.7, 0.6, 0 }  /* AI struggles */
#define DIFF_NORMAL { 1.0, 1.0, 1.0, 1.0, 1.0, 0 }  /* AI plays fair */
#define DIFF_HARD   { 1.3, 1.2, 1.5, 1.3, 1.2, 1 }  /* AI sharper */
#define DIFF_BRUTAL { 1.5, 1.3, 2.0, 2.0, 1.5, 2 }  /* AI relentless */

/* ------------------------------------------------------------------ */
/* Multi-Personality Loading                                          */
/* ------------------------------------------------------------------ */

/* Maximum number of personality slots (one per nation) */
#ifndef ABSMAXNTN
#define PERSONALITY_MAX_NATIONS 100
#else
#define PERSONALITY_MAX_NATIONS ABSMAXNTN
#endif

/* Nation-to-personality mapping. Persists across turns.
 * personality_slots[nation_id] = personality type index, or -1 if none.
 * personality_cache[type] = loaded PERSONALITY_STRUCT for that type.
 */
typedef struct s_personality_registry {
  PERSONALITY_STRUCT cache[PERSONALITY_COUNT];	/* loaded personality data	*/
  int            cache_loaded[PERSONALITY_COUNT];	/* 1 if cache[type] is valid	*/
  int            slot[PERSONALITY_MAX_NATIONS];	/* nation_id → personality type	*/
  int            count;				/* number of registered nations	*/
  DIFFICULTY_CONFIG difficulty;		/* current difficulty multipliers	*/
  DifficultyLevel  difficulty_level;	/* active difficulty level		*/
} PERSONALITY_REGISTRY_STRUCT, *PERSONALITY_REGISTRY_PTR;

/* Load all personality JSON files from the search path.
 * Returns number of personalities loaded, or -1 on error.
 * Populates registry->cache with loaded data.
 */
int personality_load_all(PERSONALITY_REGISTRY_PTR registry);

/* Assign a personality to a nation slot.
 * Returns 0 on success, -1 if nation_id or type is invalid.
 */
int personality_assign(PERSONALITY_REGISTRY_PTR registry,
                       int nation_id, PersonalityType type);

/* Get the personality for a given nation.
 * Returns pointer into registry cache, or NULL if nation has no personality.
 */
PERSONALITY_PTR personality_for_nation(PERSONALITY_REGISTRY_PTR registry,
                                        int nation_id);

/* Initialize a personality registry (zero all slots, mark cache unloaded). */
void personality_registry_init(PERSONALITY_REGISTRY_PTR registry);

/* Set difficulty level. Applies multipliers to all personality-derived values. */
void personality_set_difficulty(PERSONALITY_REGISTRY_PTR registry,
                                DifficultyLevel level);

/* Get the current difficulty config. Returns read-only pointer. */
const DIFFICULTY_CONFIG *personality_get_difficulty(PERSONALITY_REGISTRY_PTR registry);

#endif /* PERSONALITY_H */