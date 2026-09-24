#include "engine/engine.h"
extern const tileset_def_t ts_sokoban;

/* Title and "solved" screen background. Text is printed over the void bands
   (rows 2-5 and 12-13); the player sprite stands at cells (4,8), left of the crate. */
static const char * const rows[] = {
 /*           1111111111 */
 /* 01234567890123456789 */
   "####################",   /* 0 */
   "####################",   /* 1 */
   "##                ##",   /* 2 */
   "##                ##",   /* 3 */
   "##                ##",   /* 4 */
   "##                ##",   /* 5 */
   "##----------------##",   /* 6 */
   "##----------------##",   /* 7 */
   "##----BC--ab--WX--##",   /* 8 */
   "##----DE--cd--YZ--##",   /* 9 */
   "##----------------##",   /* 10 */
   "##----------------##",   /* 11 */
   "##                ##",   /* 12 */
   "##                ##",   /* 13 */
   "##----------------##",   /* 14 */
   "##----------------##",   /* 15 */
   "####################",   /* 16 */
   "####################",   /* 17 */
};

static const map_legend_t legend[] = {
    { ' ', 0, 5, 0 },                       /* void (text band) */
    { '#', 1, 0, TF_SOLID },                /* wall */
    { '-', 2, 1, 0 },                       /* floor */
    { 'a', 3, 2, 0 },                       /* goal, 4 quarters */
    { 'b', 3, 2, TF_FLIPX },
    { 'c', 3, 2, TF_FLIPY },
    { 'd', 3, 2, TF_FLIPX | TF_FLIPY },
    { 'B', 4, 3, 0 },                       /* crate */
    { 'C', 5, 3, 0 },
    { 'D', 6, 3, 0 },
    { 'E', 7, 3, 0 },
    { 'W', 4, 4, 0 },                       /* crate on goal */
    { 'X', 5, 4, 0 },
    { 'Y', 6, 4, 0 },
    { 'Z', 7, 4, 0 },
    { 0, 0, 0, 0 }
};

const map_def_t map_title = { 20, 18, rows, legend, &ts_sokoban };
