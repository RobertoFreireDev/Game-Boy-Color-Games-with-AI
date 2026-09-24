#ifndef ASSETS_H
#define ASSETS_H
#include "engine/engine.h"

/* palettes */
extern const palette_color_t pal_wh_wall[4], pal_wh_floor[4], pal_wh_goal[4],
                             pal_wh_crate[4], pal_wh_crate_ok[4], pal_wh_void[4];
extern const palette_color_t pal_player[4], pal_crate_obj[4];

/* sprites */
extern const sprite_def_t spr_player, spr_crate;

/* tilesets + maps */
extern const tileset_def_t ts_sokoban;
extern const map_def_t map_level1, map_title;

/* font */
extern const uint8_t font_main[], font_box_tiles[];

/* music */
extern const song_t mus_title, mus_puzzle, mus_solved;

/* sfx */
extern const sfx_t sfx_menu, sfx_start, sfx_step, sfx_push, sfx_bump, sfx_goal, sfx_undo;
#endif
