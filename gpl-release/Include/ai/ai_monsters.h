/* ai_monsters.h — Monster AI interface for Conquer V5
 *
 * Monster nations (lizards, savages, pirates, nomads) use fixed
 * behavior patterns, not the personality system. This header
 * provides the dispatcher and individual monster update functions.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef AI_MONSTERS_H
#define AI_MONSTERS_H

/* Dispatch the appropriate monster update based on nation active type.
 * Called from move_for_ntn() when n_ismonster(ntn_ptr->active) is true.
 * Returns 0 on success, -1 if nation is not a recognized monster type.
 */
int ai_monster_update(void);

/* Individual monster update functions.
 * Each calls monster_growth() first, then applies type-specific behavior.
 */
extern void upd_lizards(void);
extern void upd_savages(void);
extern void upd_nomads(void);
extern void upd_pirates(void);

/* Shared monster support functions */
extern void monster_growth(void);
extern void monster_move_army(int vtype, int etype);

#endif /* AI_MONSTERS_H */