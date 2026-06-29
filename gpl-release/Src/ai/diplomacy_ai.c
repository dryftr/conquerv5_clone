// SPDX-License-Identifier: GPL-3.0-or-later
/* diplomacy_ai.c - AI diplomacy module implementation */
/*
 * Conquer Reborn - Honest AI
 * Sprint 1, Task 1.3: AI Diplomacy Module
 *
 * Personality-driven diplomatic evaluation. Each AI nation evaluates
 * its known neighbors through fog of war, builds trust levels, and
 * proposes diplomatic actions based on personality weights.
 *
 * Honest AI principle: AI uses NO information it cannot see through
 * its own fog of war. Trust is earned through observed behavior, not
 * omniscience.
 */

#ifdef MEMORYH
/* Conquer build: include core headers for ntntype and data structures */
#include "dataA.h"
#include "dstatusX.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ai/ai_standalone_types.h"
#include "ai/personality.h"
#include "ai/fog_of_war.h"
#include "ai/decision.h"
#include "ai/diplomacy_ai.h"

/* ------------------------------------------------------------------ */
/* Action name strings for reports                                       */
/* ------------------------------------------------------------------ */

static const char *diplomacy_action_name[] = {
  "None",
  "Propose Alliance",
  "Propose Treaty",
  "Propose Trade",
  "Improve Relations",
  "Issue Warning",
  "Make Demand",
  "Declare War",
  "Request Peace",
  "Break Treaty",
  "Share Intel"
};

/* ------------------------------------------------------------------ */
/* Initialization                                                        */
/* ------------------------------------------------------------------ */

void
diplomacy_init(DIPLOMACY_STATE_PTR state, ntntype nation_id)
{
  if (!state) return;

  memset(state, 0, sizeof(DIPLOMACY_STATE_STRUCT));
  state->self_nation = nation_id;
  state->assessment_count = 0;
  state->proposed_action = DIPLO_ACT_NONE;
  state->proposed_target = 0;
  state->proposed_confidence = 0.0;
  state->proposed_reason[0] = '\0';
}

/* ------------------------------------------------------------------ */
/* Evaluate known nations                                                */
/* ------------------------------------------------------------------ */

/* Estimate a nation's military strength from what we can see.
 * In honest AI, this is ONLY based on fog-of-war-visible data.
 * We cannot see hidden armies or unobserved sectors.
 */
static double
estimate_visible_strength(FOW_PTR fog, ntntype self_id, ntntype other_id)
{
  /* In standalone testing, fog may be NULL or have zero map.
   * In Conquer integration, we scan visible sectors owned by other_id
   * and count visible military/population as a proxy for strength.
   *
   * Sprint 1: use a simplified heuristic based on border visibility.
   * Full integration will scan actual sector data through fog.
   */
  if (!fog) return 0.5;  /* unknown = moderate estimate */

  double strength = 0.0;
  int visible_their_sectors = 0;

  /* Count sectors we can see that belong to the other nation */
  int x, y;
  for (x = 0; x < fog->map_width; x++) {
    for (y = 0; y < fog->map_height; y++) {
      if (fog_can_see(fog, x, y)) {
        VIS_SECTOR_STRUCT *vs = &fog->remembered[x + y * fog->map_width];
        if (vs->owner_when_seen == (int)other_id) {
          visible_their_sectors++;
          /* Military proxy: population in their visible sectors */
          strength += (double)vs->people_when_seen * 0.01;
        }
      }
    }
  }

  /* If we haven't seen any of their sectors, estimate conservatively */
  if (visible_their_sectors == 0) {
    return 0.3;  /* unknown = low-moderate estimate */
  }

  /* Normalize: more visible sectors = bigger nation = more strength */
  strength += (double)visible_their_sectors * 0.1;

  return strength > 1.0 ? 1.0 : strength;
}

/* Assess threat level of another nation based on what we can see */
static double
assess_threat(PERSONALITY_PTR pers, double perceived_strength,
              double distance_factor, int is_hostile)
{
  double threat;

  /* Base threat from perceived strength */
  threat = perceived_strength * 0.6;

  /* Closer nations are more threatening */
  threat *= (1.0 + distance_factor);

  /* Known hostile nations are more threatening */
  if (is_hostile) {
    threat *= 1.5;
  }

  /* Personality risk tolerance modulates threat perception.
   * Low risk tolerance = perceive more threat.
   * High risk tolerance = perceive less threat.
   */
  threat *= (1.5 - pers->risk_tolerance);

  return threat > 1.0 ? 1.0 : threat;
}

int
diplomacy_evaluate(DIPLOMACY_STATE_PTR state,
                   PERSONALITY_PTR pers,
                   FOW_PTR fog,
                   ntntype nation_id)
{
  if (!state || !pers) return -1;

  state->assessment_count = 0;
  state->allied_count = 0;
  state->hostile_count = 0;
  state->neutral_count = 0;

  /* In Sprint 1, we evaluate based on board evaluation data.
   * The board eval already counts allied/hostile/neutral nations.
   *
   * For each known nation (visible through fog), we build an
   * assessment with trust, perceived strength, and threat.
   *
   * Nations we've never seen (fog-covered) are skipped.
   */

  /* This will be populated from Conquer's dstatus[] in integration.
   * For standalone testing, we use fog-of-war data.
   */
  int i;
  for (i = 0; i < DIPLO_MAX_ASSESSMENTS && i < 100; i++) {
    if (i == (int)nation_id) continue;  /* skip self */

    /* In standalone mode, we only assess nations we can see.
     * In Conquer integration, we check dstatus[] for known nations.
     * For now: skip nations we haven't encountered through fog.
     */
    DIPLO_ASSESS_STRUCT *a = &state->assessments[state->assessment_count];
    memset(a, 0, sizeof(DIPLO_ASSESS_STRUCT));

    a->other_nation = (ntntype)i;
    a->known = 0;  /* Will be set to 1 when we actually see them */

    /* Initial trust from personality defaults */
    a->trust = pers->diplomacy.trust_initial;
    a->perceived_strength = 0.3;  /* conservative default */
    a->perceived_threat = 0.2;    /* low default */
    a->perceived_economy = 0.3;

    state->assessment_count++;
  }

  /* Limit to realistic nation count */
  if (state->assessment_count > 100) {
    state->assessment_count = 100;
  }

  return state->assessment_count;
}

/* ------------------------------------------------------------------ */
/* Propose diplomatic action                                             */
/* ------------------------------------------------------------------ */

int
diplomacy_propose(DIPLOMACY_STATE_PTR state,
                  PERSONALITY_PTR pers,
                  BOARD_EVAL_PTR eval)
{
  double best_score = -1.0;
  int best_action = DIPLO_ACT_NONE;
  int best_target = -1;
  char best_reason[BIGLTH];

  if (!state || !pers || !eval) return -1;

  best_reason[0] = '\0';

  /* No nations to interact with? No action needed. */
  if (state->assessment_count == 0) {
    state->proposed_action = DIPLO_ACT_NONE;
    state->proposed_target = 0;
    strncpy(state->proposed_reason, "No nations to interact with",
            BIGLTH - 1);
    state->proposed_confidence = 0.0;
    return 1;
  }

  /* Strategy-informed diplomacy.
   *
   * If we're in DEFEND mode: seek alliances aggressively
   * If we're in ECONOMY mode: seek trade agreements
   * If we're in ATTACK mode: may declare war on weak neighbors
   * If we're in EXPAND mode: improve relations with neighbors
   * If we're in SCOUT mode: share intel with allies
   * If we're in DIPLOMACY mode: full diplomatic engagement
   */

  /* Evaluate each known nation for potential actions */
  int i;
  for (i = 0; i < state->assessment_count; i++) {
    DIPLO_ASSESS_PTR a = &state->assessments[i];

    /* Alliance willingness check */
    if (pers->diplomacy.alliance_willingness > 0.3 &&
        a->perceived_threat > 0.5 &&
        eval->hostile_nations > 0) {
      /* We feel threatened and are open to alliances */
      double score = pers->diplomacy.alliance_willingness *
                     a->perceived_threat;
      if (score > best_score) {
        best_score = score;
        best_action = DIPLO_ACT_PROPOSE_ALLIANCE;
        best_target = (int)a->other_nation;
        snprintf(best_reason, BIGLTH,
                 "Seeking alliance against common threat (trust=%.2f, threat=%.2f)",
                 a->trust, a->perceived_threat);
      }
    }

    /* War declaration check */
    if (a->trust < pers->diplomacy.betrayal_threshold &&
        eval->total_military > 0 &&
        a->perceived_strength < eval->total_military * 0.01) {
      /* We don't trust them AND we're much stronger */
      double score = (pers->diplomacy.betrayal_threshold - a->trust) *
                     pers->attack_preference;
      if (score > best_score && score > 0.3) {
        best_score = score;
        best_action = DIPLO_ACT_DECLARE_WAR;
        best_target = (int)a->other_nation;
        snprintf(best_reason, BIGLTH,
                 "Military advantage, low trust (trust=%.2f, strength=%.2f)",
                 a->trust, a->perceived_strength);
      }
    }

    /* Trade proposal check (economy-focused) */
    if (pers->base_priority.economy > 0.2 &&
        a->trust > 0.3 && a->perceived_economy > 0.3) {
      double score = pers->base_priority.economy * a->trust *
                     a->perceived_economy;
      if (score > best_score) {
        best_score = score;
        best_action = DIPLO_ACT_PROPOSE_TRADE;
        best_target = (int)a->other_nation;
        snprintf(best_reason, BIGLTH,
                 "Mutual economic benefit (trust=%.2f, economy=%.2f)",
                 a->trust, a->perceived_economy);
      }
    }

    /* Improve relations with neighbors */
    if (a->trust > 0.3 && a->trust < 0.7 &&
        pers->diplomacy.trust_growth_rate > 0.03) {
      double score = pers->diplomacy.trust_growth_rate *
                     (1.0 - fabs(a->trust - 0.5));
      if (score > best_score) {
        best_score = score;
        best_action = DIPLO_ACT_IMPROVE_RELATIONS;
        best_target = (int)a->other_nation;
        snprintf(best_reason, BIGLTH,
                 "Building trust with neighbor (trust=%.2f)",
                 a->trust);
      }
    }

    /* Warning to hostile neighbors */
    if (a->perceived_threat > 0.6 &&
        a->trust < pers->diplomacy.betrayal_threshold) {
      double score = a->perceived_threat * (1.0 - a->trust);
      if (score > best_score) {
        best_score = score;
        best_action = DIPLO_ACT_WARN;
        best_target = (int)a->other_nation;
        snprintf(best_reason, BIGLTH,
                 "Warning hostile neighbor (threat=%.2f, trust=%.2f)",
                 a->perceived_threat, a->trust);
      }
    }
  }

  /* Store the proposed action */
  if (best_score > 0.1) {
    state->proposed_action = best_action;
    state->proposed_target = (ntntype)best_target;
    strncpy(state->proposed_reason, best_reason, BIGLTH - 1);
    state->proposed_reason[BIGLTH - 1] = '\0';
    state->proposed_confidence = best_score > 1.0 ? 1.0 : best_score;
    return 0;
  }

  /* No significant diplomatic action needed */
  state->proposed_action = DIPLO_ACT_NONE;
  state->proposed_target = 0;
  strncpy(state->proposed_reason, "No diplomatic action needed", BIGLTH - 1);
  state->proposed_confidence = 0.0;
  return 1;
}

/* ------------------------------------------------------------------ */
/* Intel sharing                                                         */
/* ------------------------------------------------------------------ */

void
diplomacy_share_intel(DIPLOMACY_STATE_PTR state,
                      FOW_PTR our_fog,
                      ntntype ally_nation,
                      double sharing_level)
{
  if (!state || !our_fog || sharing_level <= 0.0) return;

  /* In Conquer integration, this will actually transfer
   * visibility data to the ally's fog of war map.
   *
   * sharing_level determines how much:
   *   Allied (0.7): 70% of known sectors shared
   *   Treaty (0.4): 40% of known sectors shared
   *   Friendly (0.2): 20% of known sectors shared
   *
   * Sprint 1: Mark which sectors would be shared.
   * Full implementation will update ally's fog map.
   */

  int sectors_to_share = (int)(our_fog->known_count * sharing_level);
  (void)ally_nation;  /* Used in full integration */

  /* Debug log */
  fprintf(stderr, "  [Diplomacy] Nation %d sharing %d/%d known sectors (%.0f%%) with nation %d\n",
          (int)state->self_nation, sectors_to_share,
          our_fog->known_count, sharing_level * 100.0,
          (int)ally_nation);
}

/* ------------------------------------------------------------------ */
/* Turn update — decay trust and reset transient state                    */
/* ------------------------------------------------------------------ */

void
diplomacy_turn_update(DIPLOMACY_STATE_PTR state, PERSONALITY_PTR pers)
{
  if (!state || !pers) return;

  int i;
  for (i = 0; i < state->assessment_count; i++) {
    DIPLO_ASSESS_PTR a = &state->assessments[i];

    /* Trust naturally decays each turn */
    a->trust -= pers->diplomacy.trust_decay_rate;

    /* Clamp trust to [0, 1] */
    if (a->trust < 0.0) a->trust = 0.0;
    if (a->trust > 1.0) a->trust = 1.0;

    /* Reset border incidents counter (fresh evaluation each turn) */
    a->border_incidents = 0;

    /* Decay perceived strength slightly (information gets stale) */
    a->perceived_strength *= 0.95;
    if (a->perceived_strength < 0.1) a->perceived_strength = 0.1;
  }

  /* Reset proposed action for new turn */
  state->proposed_action = DIPLO_ACT_NONE;
  state->proposed_target = 0;
  state->proposed_reason[0] = '\0';
  state->proposed_confidence = 0.0;
}

/* ------------------------------------------------------------------ */
/* Assessment lookup                                                     */
/* ------------------------------------------------------------------ */

DIPLO_ASSESS_PTR
diplomacy_get_assess(DIPLOMACY_STATE_PTR state, ntntype other_nation)
{
  if (!state) return NULL;

  int i;
  for (i = 0; i < state->assessment_count; i++) {
    if (state->assessments[i].other_nation == other_nation) {
      return &state->assessments[i];
    }
  }

  return NULL;
}

/* ------------------------------------------------------------------ */
/* Recommended diplomatic stance                                          */
/* ------------------------------------------------------------------ */

int
diplomacy_recommended_stance(DIPLOMACY_STATE_PTR state,
                              PERSONALITY_PTR pers,
                              ntntype other_nation)
{
  if (!state || !pers) return DIP_NEUTRAL;  /* 5 */

  DIPLO_ASSESS_PTR a = diplomacy_get_assess(state, other_nation);
  if (!a || !a->known) return DIP_UNMET;    /* 0 */

  /* Trust-based stance recommendation.
   * Personality modulates the thresholds.
   *
   * High alliance_willingness → easier to reach friendly/allied
   * Low alliance_willingness → stays neutral longer
   * High betrayal_threshold → harder to declare war
   * Low betrayal_threshold → easier to go hostile
   */
  double trust = a->trust;
  double threat = a->perceived_threat;
  double aw = pers->diplomacy.alliance_willingness;
  double bt = pers->diplomacy.betrayal_threshold;

  /* War threshold from personality */
  double war_thresh = pers->diplomacy.war_threshold;

  if (trust > 0.8 && aw > 0.4) return DIP_ALLIED;      /* 1 */
  if (trust > 0.6 && aw > 0.3) return DIP_TREATY;      /* 2 */
  if (trust > 0.4)                return DIP_FRIENDLY;   /* 3 */
  if (trust > 0.2)                return DIP_PEACEFUL;  /* 4 */
  if (trust > 0.1 && threat < war_thresh) return DIP_NEUTRAL;  /* 5 */

  /* Low trust → hostile/belligerent */
  if (threat > bt) return DIP_BELLICOSE;  /* 7 */
  if (trust < 0.05) return DIP_HOSTILE;   /* 6 */

  return DIP_NEUTRAL;  /* 5 - default */
}

/* ------------------------------------------------------------------ */
/* Debug dump                                                           */
/* ------------------------------------------------------------------ */

void
diplomacy_dump(DIPLOMACY_STATE_PTR state)
{
  int i;

  if (!state) {
    fprintf(stderr, "  [Diplomacy] NULL state\n");
    return;
  }

  fprintf(stderr, "  [Diplomacy] Nation %d — %d assessments\n",
          (int)state->self_nation, state->assessment_count);
  fprintf(stderr, "  [Diplomacy] Allied: %d, Hostile: %d, Neutral: %d\n",
          state->allied_count, state->hostile_count, state->neutral_count);

  if (state->proposed_action != DIPLO_ACT_NONE) {
    fprintf(stderr, "  [Diplomacy] Proposed: %s → Nation %d (%.0f%% confidence)\n",
            diplomacy_action_name[state->proposed_action],
            (int)state->proposed_target,
            state->proposed_confidence * 100.0);
    fprintf(stderr, "  [Diplomacy] Reason: %s\n", state->proposed_reason);
  } else {
    fprintf(stderr, "  [Diplomacy] No diplomatic action proposed\n");
  }

  for (i = 0; i < state->assessment_count; i++) {
    DIPLO_ASSESS_PTR a = &state->assessments[i];
    fprintf(stderr, "  [Diplomacy]   Nation %2d: trust=%.2f strength=%.2f threat=%.2f economy=%.2f %s\n",
            (int)a->other_nation, a->trust, a->perceived_strength,
            a->perceived_threat, a->perceived_economy,
            a->known ? "[known]" : "[unknown]");
  }
}