#include "engine/engine.h"

/* Palettes swapped in at run time (gfx_set_bkg_palette / gfx_set_obj_palette / text_set_colors) */

/* goal marker, bright half of the pulse (BG slot 2 alternates with pal_wh_goal) */
const palette_color_t pal_wh_goal_glow[4] = { RGB8(96,104,136), RGB8(72,80,112), RGB8(255,248,160), RGB8(248,176,48) };

/* keeper flashing gold when a puzzle is solved (OBJ slot 0 alternates with pal_player) */
const palette_color_t pal_player_gold[4] = { RGB8(0,0,0), RGB8(96,48,16), RGB8(248,200,48), RGB8(255,248,200) };

/* UI text in gold on the solved screen (BG slot 7; box background matches the void band) */
const palette_color_t pal_ui_gold[4] = { RGB8(24,24,40), RGB8(8,8,16), RGB8(216,160,48), RGB8(248,216,88) };
