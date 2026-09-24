#include "engine/engine.h"

/* BG palettes for the warehouse (ts_sokoban). Index 0 = lightest ... 3 = darkest. */
const palette_color_t pal_wh_wall[4]    = { RGB8(232,200,160), RGB8(200,120,72),  RGB8(144,64,40),   RGB8(56,24,32) };
const palette_color_t pal_wh_floor[4]   = { RGB8(96,104,136),  RGB8(72,80,112),   RGB8(56,64,96),    RGB8(32,32,56) };
/* goal: same floor colors 0/1, marker 2 = yellow fill, 3 = orange outline */
const palette_color_t pal_wh_goal[4]    = { RGB8(96,104,136),  RGB8(72,80,112),   RGB8(248,216,64),  RGB8(208,112,24) };
/* crate: 0 unused, 1 light wood, 2 wood, 3 dark wood / outline */
const palette_color_t pal_wh_crate[4]   = { RGB8(0,0,0),       RGB8(248,208,136), RGB8(208,136,64),  RGB8(88,40,24) };
/* crate on a goal: green = done */
const palette_color_t pal_wh_crate_ok[4] = { RGB8(0,0,0),      RGB8(200,248,152), RGB8(88,200,88),   RGB8(16,72,40) };
/* void outside the warehouse (matches the UI box background) */
const palette_color_t pal_wh_void[4]    = { RGB8(24,24,40),    RGB8(40,40,64),    RGB8(56,56,88),    RGB8(8,8,16) };
