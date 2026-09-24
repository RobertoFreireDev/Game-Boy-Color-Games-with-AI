#ifndef ASSETS_H
#define ASSETS_H
#include "engine/engine.h"

/* palettes */
extern const palette_color_t pal_wh_wall[4], pal_wh_floor[4], pal_wh_goal[4],
                             pal_wh_crate[4], pal_wh_crate_ok[4], pal_wh_void[4];
extern const palette_color_t pal_player[4], pal_crate_obj[4], pal_spark[4], pal_coin[4], pal_rat[4];
extern const palette_color_t pal_bn_back[4], pal_bn_wood[4], pal_bn_spike[4], pal_bn_door[4], pal_bn_fence[4];
extern const palette_color_t pal_wh_goal_glow[4], pal_player_gold[4], pal_ui_gold[4];

/* sprites */
extern const sprite_def_t spr_player, spr_crate, spr_spark, spr_coin, spr_rat;

/* tilesets + maps */
extern const tileset_def_t ts_sokoban, ts_bonus;
extern const map_def_t map_title, map_bonus;

/* levels (cell view, see map_levels.c) */
#define LEVEL_COUNT 16
#define LEVEL_H 8                           /* strings per level, 10 chars each */
extern const char * const map_levels[];
extern const map_legend_t map_levels_legend[];

/* font */
extern const uint8_t font_main[], font_box_tiles[];

/* music */
extern const song_t mus_title, mus_puzzle, mus_solved, mus_bonus, mus_gameover;

/* sfx */
extern const sfx_t sfx_menu, sfx_start, sfx_step, sfx_push, sfx_bump, sfx_goal, sfx_undo,
                   sfx_jump, sfx_coin, sfx_hurt, sfx_stomp, sfx_door;
#endif
