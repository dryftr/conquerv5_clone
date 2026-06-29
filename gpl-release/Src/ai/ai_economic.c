/* ai_economic.c — Economic AI Module for Conquer V5
 * Sprint 2 Task 2.3: Build prioritization, resource balancing, construction queues
 *
 * JSON-driven: reads personality weights and build preferences from
 * PERSONALITY_STRUCT. Falls back to hardcoded defaults if no registry.
 * Difficulty multipliers applied after personality values.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */
#ifdef MEMORYH
#include "dataA.h"
#else
#include "ai/ai_standalone_types.h"
#endif
#include "ai/ai_economic.h"
#include "ai/personality.h"
#include "ai/ai_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Build Category Names (for logging)
 * ============================================================ */

static const char *category_names[BUILD_COUNT] = {
    "Military",
    "Fortification",
    "Economy",
    "Naval",
    "Caravan"
};

const char *
ai_econ_category_name(build_category_t cat)
{
    if (cat < 0 || cat >= BUILD_COUNT) return "Unknown";
    return category_names[cat];
}

/* ============================================================
 * Hardcoded Fallback Defaults
 * ============================================================ */

static void
set_default_build_prefs(ECON_STATE_PTR state, int personality_type)
{
    switch (personality_type) {
    case ACT_OVERT:   /* Warlord */
        state->max_builds_per_turn = ECON_MAX_BUILDS_WARLORD;
        state->reserve_pct = ECON_RESERVE_WARLORD;
        state->weight_military = 0.50;
        state->weight_fortification = 0.15;
        state->weight_economy = 0.10;
        state->weight_naval = 0.15;
        state->weight_caravan = 0.10;
        break;
    case ACT_MOBILE:  /* Pioneer */
        state->max_builds_per_turn = ECON_MAX_BUILDS_PIONEER;
        state->reserve_pct = ECON_RESERVE_PIONEER;
        state->weight_military = 0.20;
        state->weight_fortification = 0.10;
        state->weight_economy = 0.40;
        state->weight_naval = 0.15;
        state->weight_caravan = 0.15;
        break;
    case ACT_KILLER:  /* Strategist */
        state->max_builds_per_turn = ECON_MAX_BUILDS_STRATEGIST;
        state->reserve_pct = ECON_RESERVE_STRATEGIST;
        state->weight_military = 0.30;
        state->weight_fortification = 0.20;
        state->weight_economy = 0.25;
        state->weight_naval = 0.15;
        state->weight_caravan = 0.10;
        break;
    case ACT_GUERRILA: /* Merchant */
        state->max_builds_per_turn = ECON_MAX_BUILDS_MERCHANT;
        state->reserve_pct = ECON_RESERVE_MERCHANT;
        state->weight_military = 0.10;
        state->weight_fortification = 0.10;
        state->weight_economy = 0.50;
        state->weight_naval = 0.15;
        state->weight_caravan = 0.15;
        break;
    case ACT_ENFORCE: /* Fortress */
        state->max_builds_per_turn = ECON_MAX_BUILDS_FORTRESS;
        state->reserve_pct = ECON_RESERVE_FORTRESS;
        state->weight_military = 0.25;
        state->weight_fortification = 0.40;
        state->weight_economy = 0.15;
        state->weight_naval = 0.10;
        state->weight_caravan = 0.10;
        break;
    default:          /* Static/Unknown — balanced */
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

/* ============================================================
 * AI_ECONOMIC_INIT — Initialize economic state from personality
 * ============================================================ */

int
ai_economic_init(ECON_STATE_PTR state, int nation_id)
{
    PERSONALITY_REGISTRY_PTR registry = NULL;
    PERSONALITY_PTR pers = NULL;
    const DIFFICULTY_CONFIG *diff = NULL;

    if (state == NULL) return -1;

    memset(state, 0, sizeof(ECON_STATE));
    state->nation_id = nation_id;

#ifdef MEMORYH
    /* Try to load from personality registry (JSON-driven) */
    registry = ai_get_registry();
    if (registry != NULL) {
        pers = personality_for_nation(registry, nation_id);
    }

    if (pers != NULL && pers->loaded) {
        /* JSON-driven build preferences */
        state->weight_military = pers->build_pref.military;
        state->weight_fortification = pers->build_pref.fortify;
        state->weight_economy = pers->build_pref.economy;
        state->weight_naval = pers->build_pref.naval;
        state->weight_caravan = pers->build_pref.caravan;

        /* JSON-driven economic parameters */
        state->reserve_pct = pers->reserve_pct;
        state->max_builds_per_turn = pers->max_builds_per_turn;

        fprintf(fupdate,
                "  ECON: Nation %d loaded from personality '%s' "
                "(reserve=%.0f%%, max_builds=%d)\n",
                nation_id, pers->name,
                pers->reserve_pct * 100.0,
                pers->max_builds_per_turn);
    } else
#endif
    {
        /* Fallback: hardcoded defaults from personality type */
        int personality_type = ACT_OVERT;
#ifdef MEMORYH
        if (ntn_ptr != NULL && ntn_ptr->active > 0) {
            personality_type = ntn_ptr->active;
        }
#endif
        set_default_build_prefs(state, personality_type);
    }

    /* Apply difficulty multipliers */
    if (registry != NULL) {
        diff = personality_get_difficulty(registry);
        if (diff != NULL) {
            state->economy_mult = diff->economy_mult;
            state->build_cap_mult = diff->build_cap_mult;
            /* Scale max builds by difficulty */
            state->max_builds_per_turn = (int)(
                (double)state->max_builds_per_turn * diff->build_cap_mult);
            if (state->max_builds_per_turn < 1) state->max_builds_per_turn = 1;
            if (state->max_builds_per_turn > 8) state->max_builds_per_turn = 8;
        }
    }

    return 0;
}

/* ============================================================
 * AI_ECONOMIC_EVALUATE — Assess nation's economy
 * ============================================================ */

int
ai_economic_evaluate(ECON_STATE_PTR state)
{
    if (state == NULL) return -1;

#ifdef MEMORYH
    /* Read economy from game state */
    state->treasury = (long)ntn_ptr->treasury;

    /* Estimate income: sum of sector production */
    {
        int x, y;
        long income = 0;
        long expenses = 0;

        for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
            for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
                if (!XY_ONMAP(x, y)) continue;
                if (sct[x][y].owner != state->nation_id) continue;
                /* Income from production */
                income += (long)sct[x][y].people;
            }
        }

        /* Military upkeep: count armies */
        if (ntn_ptr->army_list != NULL) {
            ARMY_PTR aptr;
            for (aptr = ntn_ptr->army_list; aptr != NULL; aptr = aptr->next) {
                expenses += 1;  /* Base upkeep per army */
            }
        }

        state->income = (long)(income * state->economy_mult);
        state->expenses = expenses;
        state->net_income = state->income - state->expenses;
    }

    /* Compute spendable budget */
    state->reserve_amount = (long)((double)state->treasury * state->reserve_pct);
    state->spend_budget = state->treasury - state->reserve_amount;
    if (state->spend_budget < 0) state->spend_budget = 0;

    /* Detect situational flags */
    state->under_attack = 0;
    state->low_troops = 0;
    state->deficit = (state->net_income < 0) ? 1 : 0;

    /* Check if under attack: any enemy armies on our territory? */
    {
        int x, y;
        for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge; x++) {
            for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge; y++) {
                if (!XY_ONMAP(x, y)) continue;
                if (sct[x][y].owner == state->nation_id) {
                    /* Check for enemy armies in adjacent sectors */
                    int dx, dy;
                    for (dx = -1; dx <= 1; dx++) {
                        for (dy = -1; dy <= 1; dy++) {
                            int nx = x + dx, ny = y + dy;
                            if (!XY_ONMAP(nx, ny)) continue;
                            if (sct[nx][ny].owner != state->nation_id &&
                                sct[nx][ny].owner != UNOWNED &&
                                ai_enemy_strength(nx, ny, state->nation_id) > 0) {
                                state->under_attack = 1;
                                break;
                            }
                        }
                        if (state->under_attack) break;
                    }
                    if (state->under_attack) break;
                }
            }
            if (state->under_attack) break;
        }
    }

    /* Check troop count */
    if (ntn_ptr->army_list == NULL) {
        state->low_troops = 1;
    }

    fprintf(fupdate,
            "  ECON: Nation %d — treasury=%ld income=%ld expenses=%ld "
            "net=%ld budget=%ld reserve=%ld (reserve_pct=%.0f%%) "
            "attacked=%d low_troops=%d deficit=%d\n",
            state->nation_id, state->treasury, state->income, state->expenses,
            state->net_income, state->spend_budget, state->reserve_amount,
            state->reserve_pct * 100.0,
            state->under_attack, state->low_troops, state->deficit);
#else
    /* Standalone: no game state to read */
    state->treasury = 1000;
    state->income = 100;
    state->expenses = 50;
    state->net_income = 50;
    state->reserve_amount = (long)((double)state->treasury * state->reserve_pct);
    state->spend_budget = state->treasury - state->reserve_amount;
#endif

    return 0;
}

/* ============================================================
 * AI_ECONOMIC_SCORE_BUILDS — Prioritize build targets
 * ============================================================ */

int
ai_economic_score_builds(ECON_STATE_PTR state,
                         ECON_TARGET_PTR targets, int max_targets)
{
    int count = 0;
    double weights[BUILD_COUNT];
    int situational[BUILD_COUNT];

    if (state == NULL || targets == NULL) return 0;

    /* Weight array for scoring */
    weights[BUILD_MILITARY] = state->weight_military;
    weights[BUILD_FORTIFICATION] = state->weight_fortification;
    weights[BUILD_ECONOMY] = state->weight_economy;
    weights[BUILD_NAVAL] = state->weight_naval;
    weights[BUILD_CARAVAN] = state->weight_caravan;

    /* Situational modifiers */
    memset(situational, 0, sizeof(situational));
    if (state->under_attack)  situational[BUILD_FORTIFICATION] += ECON_MOD_UNDER_ATTACK;
    if (state->low_troops)     situational[BUILD_MILITARY] += ECON_MOD_LOW_TROOPS;
    if (state->deficit)        situational[BUILD_ECONOMY] += ECON_MOD_DEFICIT;

    /* If economy is strong (net income > expenses * 2), shift toward military */
    if (state->net_income > state->expenses * 2) {
        situational[BUILD_MILITARY] += ECON_MOD_STRONG_ECONOMY;
    }

#ifdef MEMORYH
    /* Score each owned sector for each build category */
    {
        int x, y;
        for (x = ntn_ptr->leftedge; x <= ntn_ptr->rightedge && count < max_targets; x++) {
            for (y = ntn_ptr->topedge; y <= ntn_ptr->bottomedge && count < max_targets; y++) {
                if (!XY_ONMAP(x, y)) continue;
                if (sct[x][y].owner != state->nation_id) continue;

                /* For each viable build category, compute a score */
                int cat;
                for (cat = 0; cat < BUILD_COUNT && count < max_targets; cat++) {
                    int base_score = (int)(weights[cat] * 100.0);
                    int final_score = base_score + situational[cat];

                    /* Skip zero-score builds */
                    if (final_score <= 0) continue;

                    /* Skip if we've hit our build cap */
                    if (state->builds_started >= state->max_builds_per_turn) break;

                    targets[count].x = x;
                    targets[count].y = y;
                    targets[count].category = (build_category_t)cat;
                    targets[count].base_score = base_score;
                    targets[count].situational_bonus = situational[cat];
                    targets[count].final_score = final_score;
                    targets[count].cost = 10; /* Base cost, TODO: actual build costs */
                    targets[count].name = ai_econ_category_name((build_category_t)cat);
                    count++;
                }
            }
        }
    }
#endif

    /* Sort by final_score descending (simple selection sort, n is small) */
    {
        int i, j;
        for (i = 0; i < count - 1; i++) {
            int best = i;
            for (j = i + 1; j < count; j++) {
                if (targets[j].final_score > targets[best].final_score) {
                    best = j;
                }
            }
            if (best != i) {
                ECON_TARGET tmp = targets[i];
                targets[i] = targets[best];
                targets[best] = tmp;
            }
        }
    }

#ifdef MEMORYH
    fprintf(fupdate,
            "  ECON: Nation %d scored %d build targets (max=%d, cap=%d)\n",
            state->nation_id, count, max_targets, state->max_builds_per_turn);
#endif

    return count;
}

/* ============================================================
 * AI_ECONOMIC_EXECUTE — Start builds up to budget
 * ============================================================ */

int
ai_economic_execute(ECON_STATE_PTR state)
{
    int builds = 0;
    int skipped = 0;

    if (state == NULL) return 0;

#ifdef MEMORYH
    /* Allocate target buffer */
    int max_targets = 128;
    ECON_TARGET_PTR targets = (ECON_TARGET_PTR)malloc(
        max_targets * sizeof(ECON_TARGET));
    if (targets == NULL) return 0;

    /* Evaluate economy */
    ai_economic_evaluate(state);

    /* Score and prioritize builds */
    int target_count = ai_economic_score_builds(state, targets, max_targets);

    /* Execute top builds within budget and cap */
    int i;
    for (i = 0; i < target_count && builds < state->max_builds_per_turn; i++) {
        ECON_TARGET_PTR tgt = &targets[i];

        /* Budget check: can we afford this build? */
        if (tgt->cost > state->spend_budget) {
            skipped++;
            continue;
        }

        /* Deduct from budget */
        state->spend_budget -= tgt->cost;

        /* Execute the build via action_expand */
        /* TODO: Wire to actual build system (action_expand_execute) */
        builds++;
        state->builds_started++;

        fprintf(fupdate,
                "  ECON: Nation %d builds %s at (%d,%d) score=%d cost=%ld "
                "(base=%d situational=%d) budget_remaining=%ld\n",
                state->nation_id, tgt->name, tgt->x, tgt->y,
                tgt->final_score, tgt->cost,
                tgt->base_score, tgt->situational_bonus,
                state->spend_budget);
    }

    state->builds_skipped = skipped;

    fprintf(fupdate,
            "  ECON: Nation %d economic phase complete — "
            "started=%d skipped=%d budget_remaining=%ld\n",
            state->nation_id, builds, skipped, state->spend_budget);

    free(targets);
#endif

    return builds;
}