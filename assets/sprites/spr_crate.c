#include "engine/engine.h"

/* Crate drawn as a sprite only while it slides (then it goes back to the BG).
   Same pixels as ts_sokoban tiles 4-7; they never use index 0, so nothing is transparent.
   0 = transparent, 1 = light wood, 2 = wood, 3 = dark wood / outline */
const palette_color_t pal_crate_obj[4] = { RGB8(0,0,0), RGB8(248,208,136), RGB8(208,136,64), RGB8(88,40,24) };

const uint8_t spr_crate_tiles[] = {
    /* tile 0 (top-left) */
    PX(3,3,3,3,3,3,3,3), PX(3,1,1,1,1,1,1,1), PX(3,1,2,2,2,2,2,2), PX(3,1,2,3,3,3,3,3),
    PX(3,1,2,3,2,2,3,3), PX(3,1,2,3,1,2,2,3), PX(3,1,2,3,3,1,2,2), PX(3,1,2,3,3,3,1,2),
    /* tile 1 (top-right) */
    PX(3,3,3,3,3,3,3,3), PX(1,1,1,1,1,1,2,3), PX(2,2,2,2,2,2,3,3), PX(3,3,3,3,3,2,3,3),
    PX(3,3,3,3,3,2,3,3), PX(3,3,3,3,3,2,3,3), PX(3,3,3,3,3,2,3,3), PX(2,3,3,3,3,2,3,3),
    /* tile 2 (bottom-left) */
    PX(3,1,2,3,3,3,3,1), PX(3,1,2,3,3,3,3,3), PX(3,1,2,3,3,3,3,3), PX(3,1,2,3,3,3,3,3),
    PX(3,1,2,3,3,3,3,3), PX(3,1,2,2,2,2,2,2), PX(3,2,3,3,3,3,3,3), PX(3,3,3,3,3,3,3,3),
    /* tile 3 (bottom-right) */
    PX(2,2,3,3,3,2,3,3), PX(1,2,2,3,3,2,3,3), PX(3,1,2,2,3,2,3,3), PX(3,3,1,2,3,2,3,3),
    PX(3,3,3,3,3,2,3,3), PX(2,2,2,2,2,2,3,3), PX(3,3,3,3,3,3,3,3), PX(3,3,3,3,3,3,3,3),
};

const sprite_def_t spr_crate = { spr_crate_tiles, 2, 2, 1, pal_crate_obj, 1 };
