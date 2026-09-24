#ifndef GAME_BOARD_H
#define GAME_BOARD_H
#include "engine/engine.h"

/* Sokoban board: one cell = 16x16 px = 2x2 map tiles. */
#define BOARD_W 10
#define BOARD_H 9
#define BOARD_SIZE (BOARD_W * BOARD_H)

#define CELL_WALL 0x01
#define CELL_GOAL 0x02
#define CELL_BOX  0x04

#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_RIGHT 3

#define MOVE_BLOCKED 0
#define MOVE_WALK    1
#define MOVE_PUSH    2

#define UNDO_NONE    0
#define UNDO_PUSHED  0x04   /* returned by board_undo together with UNDO_DONE | dir */
#define UNDO_DONE    0x80

extern uint8_t board[BOARD_SIZE];
extern uint8_t player_cell;
extern const int8_t dir_off[4];

void board_load(void);                  /* reads the currently loaded map (map_load first) */
void board_draw_cell(uint8_t i);        /* redraws the 2x2 BG tiles of a cell from its state */
uint8_t board_try_move(uint8_t dir);    /* MOVE_*; updates state + undo history, draws nothing */
uint8_t board_undo(void);               /* UNDO_NONE or UNDO_DONE | dir | UNDO_PUSHED; redraws cells */
uint8_t board_solved(void);
int16_t cell_px_x(uint8_t i);           /* world pixel of a cell's top-left */
int16_t cell_px_y(uint8_t i);
#endif
