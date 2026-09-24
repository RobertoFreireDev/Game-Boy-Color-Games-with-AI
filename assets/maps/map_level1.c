#include "engine/engine.h"
extern const tileset_def_t ts_sokoban;

/* The only level. One Sokoban cell = 2x2 characters; the game reads each cell
   from its top-left character (see src/game/board.c).

   Cell view (10x9), '-' floor, '.' goal, '$' crate, '@' player:
       "          "
       " ######## "
       " #--#---# "
       " #-$--$-# "
       " #--##--# "
       " #.$@-$.# "
       " #--..--# "
       " ######## "
       "          "
   Solvable in 39 moves (checked with a BFS solver).
   Row 17 is covered by the HUD window. */
static const char * const rows[] = {
 /*           1111111111 */
 /* 01234567890123456789 */
   "                    ",   /* 0 */
   "                    ",   /* 1 */
   "  ################  ",   /* 2 */
   "  ################  ",   /* 3 */
   "  ##----##------##  ",   /* 4 */
   "  ##----##------##  ",   /* 5 */
   "  ##--BC----BC--##  ",   /* 6 */
   "  ##--DE----DE--##  ",   /* 7 */
   "  ##----####----##  ",   /* 8 */
   "  ##----####----##  ",   /* 9 */
   "  ##abBCP---BCab##  ",   /* 10 */
   "  ##cdDE----DEcd##  ",   /* 11 */
   "  ##----abab----##  ",   /* 12 */
   "  ##----cdcd----##  ",   /* 13 */
   "  ################  ",   /* 14 */
   "  ################  ",   /* 15 */
   "                    ",   /* 16 */
   "                    ",   /* 17 */
};

static const map_legend_t legend[] = {
    { ' ', 0, 5, 0 },                       /* void */
    { '#', 1, 0, TF_SOLID },                /* wall */
    { '-', 2, 1, 0 },                       /* floor */
    { 'P', 2, 1, TF_SPAWN },                /* player start (drawn as floor) */
    { 'a', 3, 2, 0 },                       /* goal, 4 quarters */
    { 'b', 3, 2, TF_FLIPX },
    { 'c', 3, 2, TF_FLIPY },
    { 'd', 3, 2, TF_FLIPX | TF_FLIPY },
    { 'B', 4, 3, 0 },                       /* crate, 4 quarters */
    { 'C', 5, 3, 0 },
    { 'D', 6, 3, 0 },
    { 'E', 7, 3, 0 },
    { 'W', 4, 4, 0 },                       /* crate on goal, 4 quarters */
    { 'X', 5, 4, 0 },
    { 'Y', 6, 4, 0 },
    { 'Z', 7, 4, 0 },
    { 0, 0, 0, 0 }
};

const map_def_t map_level1 = { 20, 18, rows, legend, &ts_sokoban };
