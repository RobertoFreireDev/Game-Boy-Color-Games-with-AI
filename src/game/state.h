#ifndef STATE_H
#define STATE_H
#include "engine/engine.h"
#include "assets.h"

/* Game state that survives scene changes */
extern uint8_t game_level;                 /* level being played, 0..LEVEL_COUNT-1 */
extern uint8_t game_unlocked;              /* highest level the player may select */
extern uint16_t game_best[LEVEL_COUNT];    /* fewest moves per level, 0 = not solved yet */
extern uint16_t game_moves, game_pushes;   /* result of the last solved puzzle */
extern uint8_t game_new_best;              /* last solve beat the previous best */

void save_load(void);                      /* read progress from battery RAM (call once at boot) */
void game_record_solve(void);              /* store game_moves for game_level, unlock the next, save */
#endif
