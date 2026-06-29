// SPDX-License-Identifier: GPL-3.0-or-later
/* personality.c - AI personality loader and management */
/*
 * Conquer Reborn - Honest AI Personality System
 * Sprint 0: Warlord proof of concept
 *
 * Loads personality data from JSON files in Auxil/personalities/
 * Uses a minimal JSON parser (no external dependencies).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ai/personality.h"

/* ------------------------------------------------------------------ */
/* Minimal JSON Parser - enough for flat key-value objects             */
/* ------------------------------------------------------------------ */

/* Skip whitespace in a string, return pointer to next non-space */
static const char *
skip_ws(const char *p)
{
  while (*p && isspace((unsigned char)*p)) p++;
  return p;
}

/* Read a JSON string value into dst (max dstlen-1 chars). 
 * p should point to the opening quote.
 * Returns pointer past closing quote, or NULL on error.
 */
static const char *
read_json_string(const char *p, char *dst, int dstlen)
{
  if (!p || *p != '"') return NULL;
  p++; /* skip opening quote */
  int i = 0;
  while (*p && *p != '"' && i < dstlen - 1) {
    if (*p == '\\' && *(p+1)) {
      p++; /* skip backslash, take next char */
    }
    dst[i++] = *p++;
  }
  dst[i] = '\0';
  if (*p == '"') p++; /* skip closing quote */
  return p;
}

/* Read a JSON number (double) from string p.
 * Returns pointer past the number, or NULL on error.
 */
static const char *
read_json_number(const char *p, double *val)
{
  if (!p || (!isdigit((unsigned char)*p) && *p != '-' && *p != '.')) return NULL;
  char *end;
  *val = strtod(p, &end);
  return end;
}

/* Find a key in a JSON object string.
 * Returns pointer to the value (after colon), or NULL if not found.
 */
static const char *
find_key(const char *json, const char *key)
{
  char search[256];
  snprintf(search, sizeof(search), "\"%s\"", key);
  const char *p = strstr(json, search);
  if (!p) return NULL;
  p += strlen(search);
  p = skip_ws(p);
  if (*p != ':') return NULL;
  p++;
  p = skip_ws(p);
  return p;
}

/* Read a double value for a given key. Returns 0 if found, -1 if not. */
static int
pers_get_double(const char *json, const char *key, double *val)
{
  const char *p = find_key(json, key);
  if (!p) return -1;
  double v;
  const char *end = read_json_number(p, &v);
  if (!end) return -1;
  *val = v;
  return 0;
}

/* Read a string value for a given key. Returns 0 if found, -1 if not. */
static int
pers_get_string(const char *json, const char *key, char *dst, int dstlen)
{
  const char *p = find_key(json, key);
  if (!p) return -1;
  p = skip_ws(p);
  return read_json_string(p, dst, dstlen) ? 0 : -1;
}


/* Read an integer value for a given key. Returns 0 if found, -1 if not. */
static int
pers_get_int(const char *json, const char *key, int *val)
{
  double d;
  if (pers_get_double(json, key, &d) != 0) return -1;
  *val = (int)d;
  return 0;
}

/* ------------------------------------------------------------------ */
/* Personality Name/Type Mapping                                       */
/* ------------------------------------------------------------------ */

static const char *personality_names[] = {
  "Warlord",
  "Fortress",
  "Merchant",
  "Pioneer",
  "Strategist",
  NULL
};

const char *
personality_name_from_type(PersonalityType type)
{
  if (type < 0 || type >= PERSONALITY_COUNT) return NULL;
  return personality_names[type];
}

PersonalityType
personality_type_from_name(const char *name)
{
  if (!name) return -1;
  for (int i = 0; i < PERSONALITY_COUNT; i++) {
    if (strcasecmp(name, personality_names[i]) == 0) return (PersonalityType)i;
  }
  return -1;
}

/* ------------------------------------------------------------------ */
/* Load personality from JSON file                                     */
/* ------------------------------------------------------------------ */

int
personality_load(const char *name, PERSONALITY_PTR pers)
{
  if (!name || !pers) return -1;
  
  /* Zero out the struct */
  memset(pers, 0, sizeof(PERSONALITY_STRUCT));
  
  /* Resolve personality type from name */
  PersonalityType type = personality_type_from_name(name);
  if (type < 0) {
    fprintf(stderr, "personality_load: unknown personality '%s'\n", name);
    return -1;
  }
  pers->type = type;
  
  /* Build file path: Auxil/personalities/<name>.json */
  /* Lowercase the name for filename */
  char filename[FILELTH];
  char lname[NAMELTH+1];
  int i;
  for (i = 0; name[i] && i < NAMELTH; i++) {
    lname[i] = tolower((unsigned char)name[i]);
  }
  lname[i] = '\0';
  snprintf(filename, sizeof(filename), "Auxil/personalities/%s.json", lname);
  
  /* Open and read the file */
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    fprintf(stderr, "personality_load: cannot open '%s'\n", filename);
    return -1;
  }
  
  /* Read entire file into buffer */
  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  
  if (fsize <= 0 || fsize > 65536) {
    fclose(fp);
    fprintf(stderr, "personality_load: file '%s' size %ld invalid\n",
            filename, fsize);
    return -1;
  }
  
  char *json = (char *)malloc(fsize + 1);
  if (!json) {
    fclose(fp);
    fprintf(stderr, "personality_load: malloc failed\n");
    return -1;
  }
  
  size_t nread = fread(json, 1, fsize, fp);
  fclose(fp);
  json[nread] = '\0';
  
  /* Parse top-level string fields */
  pers_get_string(json, "name", pers->name, sizeof(pers->name));
  pers_get_string(json, "description", pers->description, sizeof(pers->description));
  
  /* Parse core personality parameters */
  pers_get_double(json, "risk_tolerance", &pers->risk_tolerance);
  pers_get_double(json, "reaction_speed", &pers->reaction_speed);
  pers_get_double(json, "adaptation_rate", &pers->adaptation_rate);
  pers_get_double(json, "patience", &pers->patience);
  
  /* Parse attack/expansion parameters */
  pers_get_double(json, "attack_preference", &pers->attack_preference);
  pers_get_double(json, "retreat_threshold", &pers->retreat_threshold);
  pers_get_double(json, "siege_patience", &pers->siege_patience);
  pers_get_double(json, "expansion_aggression", &pers->expansion_aggression);
  pers_get_double(json, "territory_focus", &pers->territory_focus);

  /* Parse economic parameters (Sprint 2.3) */
  pers_get_int(json, "garrison_threshold", &pers->garrison_threshold);
  pers_get_double(json, "reserve_pct", &pers->reserve_pct);
  pers_get_int(json, "max_builds_per_turn", &pers->max_builds_per_turn);

  /* Parse base_priority weights */
  pers_get_double(json, "priority_military", &pers->base_priority.military);
  pers_get_double(json, "priority_economy", &pers->base_priority.economy);
  pers_get_double(json, "priority_expansion", &pers->base_priority.expansion);
  pers_get_double(json, "priority_defense", &pers->base_priority.defense);
  pers_get_double(json, "priority_diplomacy", &pers->base_priority.diplomacy);
  pers_get_double(json, "priority_scouting", &pers->base_priority.scouting);
  pers_get_double(json, "priority_research", &pers->base_priority.research);
  
  /* Copy base priorities to effective (will be adapted at runtime) */
  pers->effective_priority = pers->base_priority;
  
  /* Parse build preferences */
  pers_get_double(json, "build_military_units", &pers->build_pref.military_units);
  pers_get_double(json, "build_fortifications", &pers->build_pref.fortifications);
  pers_get_double(json, "build_economic_buildings", &pers->build_pref.economic_buildings);
  pers_get_double(json, "build_naval_units", &pers->build_pref.naval_units);
  pers_get_double(json, "build_caravans", &pers->build_pref.caravans);
  
  /* Parse starting allocation */
  pers_get_double(json, "startalloc_military", &pers->start_alloc.military_pct);
  pers_get_double(json, "startalloc_economy", &pers->start_alloc.economy_pct);
  pers_get_double(json, "startalloc_expansion", &pers->start_alloc.expansion_pct);
  pers_get_double(json, "startalloc_defense", &pers->start_alloc.defense_pct);
  pers_get_double(json, "startalloc_scouting", &pers->start_alloc.scouting_pct);
  
  /* Parse scout configuration */
  pers_get_double(json, "scout_pct", &pers->scout_cfg.scout_pct);
  pers_get_int(json, "scout_range", &pers->scout_cfg.scout_range);
  pers_get_double(json, "scout_explore_unknown_weight", &pers->scout_cfg.explore_unknown_weight);
  pers_get_double(json, "scout_revisit_stale_weight", &pers->scout_cfg.revisit_stale_weight);
  pers_get_double(json, "scout_border_priority_weight", &pers->scout_cfg.border_priority_weight);
  
  /* Parse diplomacy configuration */
  pers_get_double(json, "diplomacy_trust_initial", &pers->diplomacy.trust_initial);
  pers_get_double(json, "diplomacy_trust_growth_rate", &pers->diplomacy.trust_growth_rate);
  pers_get_double(json, "diplomacy_trust_decay_rate", &pers->diplomacy.trust_decay_rate);
  pers_get_double(json, "diplomacy_betrayal_threshold", &pers->diplomacy.betrayal_threshold);
  pers_get_double(json, "diplomacy_alliance_willingness", &pers->diplomacy.alliance_willingness);
  pers_get_double(json, "diplomacy_war_threshold", &pers->diplomacy.war_threshold);
  
  /* Default stance for diplomacy - stored as int matching Diplotype enum values.
   * DIP_NEUTRAL=5 in dstatusX.h. We use the numeric value directly. */
  int stance_int = 5; /* DIP_NEUTRAL */
  pers_get_int(json, "diplomacy_default_stance", &stance_int);
  if (stance_int >= 0 && stance_int <= 9) { /* valid Diplotype range */
    pers->diplomacy.default_stance = stance_int;
  } else {
    pers->diplomacy.default_stance = 5; /* DIP_NEUTRAL */
  }
  
  free(json);
  
  /* Mark as loaded */
  pers->loaded = 1;
  
  /* Validate */
  if (personality_validate(pers) != 0) {
    fprintf(stderr, "personality_load: validation failed for '%s'\n", name);
    pers->loaded = 0;
    return -1;
  }
  
  return 0;
}

/* ------------------------------------------------------------------ */
/* Effective Weight Calculation                                         */
/* ------------------------------------------------------------------ */

void
personality_get_effective_weight(PERSONALITY_PTR pers,
                                  PRIORITY_PTR situational,
                                  PRIORITY_PTR result)
{
  if (!pers || !situational || !result) return;
  
  double reaction = pers->reaction_speed;
  double base_weight = 1.0 - reaction;
  
  result->military   = pers->base_priority.military   * base_weight + situational->military   * reaction;
  result->economy    = pers->base_priority.economy    * base_weight + situational->economy    * reaction;
  result->expansion  = pers->base_priority.expansion  * base_weight + situational->expansion  * reaction;
  result->defense    = pers->base_priority.defense    * base_weight + situational->defense    * reaction;
  result->diplomacy  = pers->base_priority.diplomacy  * base_weight + situational->diplomacy  * reaction;
  result->scouting   = pers->base_priority.scouting   * base_weight + situational->scouting   * reaction;
  result->research   = pers->base_priority.research   * base_weight + situational->research   * reaction;
}

/* ------------------------------------------------------------------ */
/* Validation                                                          */
/* ------------------------------------------------------------------ */

int
personality_validate(PERSONALITY_PTR pers)
{
  if (!pers || !pers->loaded) return -1;
  
  /* Name must be non-empty */
  if (pers->name[0] == '\0') return -1;
  
  /* Type must be in range */
  if (pers->type < 0 || pers->type >= PERSONALITY_COUNT) return -1;
  
  /* Priority weights should be non-negative */
  if (pers->base_priority.military < 0 ||
      pers->base_priority.economy < 0 ||
      pers->base_priority.expansion < 0 ||
      pers->base_priority.defense < 0 ||
      pers->base_priority.diplomacy < 0 ||
      pers->base_priority.scouting < 0) return -1;
  
  /* Core parameters must be in [0, 1] range */
  if (pers->risk_tolerance < 0 || pers->risk_tolerance > 1) return -1;
  if (pers->reaction_speed < 0 || pers->reaction_speed > 1) return -1;
  if (pers->adaptation_rate < 0 || pers->adaptation_rate > 1) return -1;
  
  return 0;
}

/* ------------------------------------------------------------------ */
/* Debug Dump                                                          */
/* ------------------------------------------------------------------ */

void
personality_dump(PERSONALITY_PTR pers)
{
  if (!pers) {
    fprintf(stderr, "personality_dump: NULL pointer\n");
    return;
  }
  
  fprintf(stderr, "=== Personality: %s (type %d) ===\n",
          pers->name, pers->type);
  fprintf(stderr, "  Description: %s\n", pers->description);
  fprintf(stderr, "  Risk tolerance: %.2f\n", pers->risk_tolerance);
  fprintf(stderr, "  Reaction speed: %.2f\n", pers->reaction_speed);
  fprintf(stderr, "  Adaptation rate: %.2f\n", pers->adaptation_rate);
  fprintf(stderr, "  Patience: %.1f turns\n", pers->patience);
  fprintf(stderr, "  Attack pref: %.2f, Retreat thresh: %.2f\n",
          pers->attack_preference, pers->retreat_threshold);
  fprintf(stderr, "  Base priorities: mil=%.2f eco=%.2f exp=%.2f def=%.2f dip=%.2f sc=%.2f res=%.2f\n",
          pers->base_priority.military,
          pers->base_priority.economy,
          pers->base_priority.expansion,
          pers->base_priority.defense,
          pers->base_priority.diplomacy,
          pers->base_priority.scouting,
          pers->base_priority.research);
  fprintf(stderr, "  Loaded: %s\n", pers->loaded ? "YES" : "NO");
}

/* ------------------------------------------------------------------ */
/* Multi-Personality Registry                                          */
/* ------------------------------------------------------------------ */

void
personality_registry_init(PERSONALITY_REGISTRY_PTR registry)
{
  if (!registry) return;
  memset(registry, 0, sizeof(PERSONALITY_REGISTRY_STRUCT));
  int i;
  for (i = 0; i < PERSONALITY_MAX_NATIONS; i++) {
    registry->slot[i] = -1;  /* no personality assigned */
  }
  for (i = 0; i < PERSONALITY_COUNT; i++) {
    registry->cache_loaded[i] = 0;
  }
  registry->count = 0;
}

int
personality_load_all(PERSONALITY_REGISTRY_PTR registry)
{
  if (!registry) return -1;

  int loaded = 0;
  int i;
  for (i = 0; i < PERSONALITY_COUNT; i++) {
    const char *name = personality_name_from_type((PersonalityType)i);
    if (!name) continue;

    if (personality_load(name, &registry->cache[i]) != 0) {
      fprintf(stderr, "personality_load_all: failed to load %s\n", name);
      continue;
    }
    registry->cache_loaded[i] = 1;
    loaded++;
  }

  return loaded;
}

int
personality_assign(PERSONALITY_REGISTRY_PTR registry,
                    int nation_id, PersonalityType type)
{
  if (!registry) return -1;
  if (nation_id < 0 || nation_id >= PERSONALITY_MAX_NATIONS) return -1;
  if (type < 0 || type >= PERSONALITY_COUNT) return -1;
  if (!registry->cache_loaded[type]) return -1;

  registry->slot[nation_id] = type;
  registry->count++;
  return 0;
}

PERSONALITY_PTR
personality_for_nation(PERSONALITY_REGISTRY_PTR registry,
                        int nation_id)
{
  if (!registry) return NULL;
  if (nation_id < 0 || nation_id >= PERSONALITY_MAX_NATIONS) return NULL;

  int type = registry->slot[nation_id];
  if (type < 0 || type >= PERSONALITY_COUNT) return NULL;
  if (!registry->cache_loaded[type]) return NULL;

  return &registry->cache[type];
}

/* ------------------------------------------------------------------ */
/* Difficulty Configuration                                             */
/* ------------------------------------------------------------------ */

static const DIFFICULTY_CONFIG difficulty_presets[DIFFICULTY_COUNT] = {
  DIFF_EASY,
  DIFF_NORMAL,
  DIFF_HARD,
  DIFF_BRUTAL
};

void
personality_set_difficulty(PERSONALITY_REGISTRY_PTR registry,
                            DifficultyLevel level)
{
  if (!registry) return;
  if (level < 0 || level >= DIFFICULTY_COUNT) level = DIFFICULTY_NORMAL;

  registry->difficulty_level = level;
  registry->difficulty = difficulty_presets[level];
}

const DIFFICULTY_CONFIG *
personality_get_difficulty(PERSONALITY_REGISTRY_PTR registry)
{
  if (!registry) return NULL;
  return &registry->difficulty;
}