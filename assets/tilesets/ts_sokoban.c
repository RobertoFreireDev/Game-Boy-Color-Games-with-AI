#include "engine/engine.h"

extern const palette_color_t pal_wh_wall[4], pal_wh_floor[4], pal_wh_goal[4],
                             pal_wh_crate[4], pal_wh_crate_ok[4], pal_wh_void[4];

/* Warehouse tiles. A Sokoban cell is 16x16 = 2x2 tiles.
   BG palette slots: 0 wall, 1 floor, 2 goal, 3 crate, 4 crate on goal, 5 void */
const uint8_t ts_sokoban_tiles[] = {
    /* 0: void (outside), palette 5 */
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,1,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,1,0),
    PX(0,0,0,0,0,0,0,0),
    /* 1: wall brick (seamless), palette 0 */
    PX(3,3,3,3,3,3,3,3),
    PX(0,0,0,3,0,0,0,0),
    PX(1,1,2,3,0,1,1,1),
    PX(2,2,2,3,1,2,2,2),
    PX(3,3,3,3,3,3,3,3),
    PX(0,0,0,0,0,0,0,3),
    PX(0,1,1,1,1,1,2,3),
    PX(1,2,2,2,2,2,2,3),
    /* 2: floor slab (grout on right column and bottom row), palette 1 */
    PX(0,0,0,0,0,0,0,1),
    PX(0,0,0,0,0,0,0,1),
    PX(0,0,1,0,0,0,0,1),
    PX(0,0,0,0,0,0,0,1),
    PX(0,0,0,0,0,0,0,1),
    PX(0,0,0,0,0,0,0,1),
    PX(0,0,0,0,0,0,0,1),
    PX(1,1,1,1,1,1,1,1),
    /* 3: goal diamond, top-left quarter (flip X/Y for the others), palette 2 */
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,3),
    PX(0,0,0,0,0,0,3,2),
    PX(0,0,0,0,0,3,2,2),
    PX(0,0,0,0,3,2,2,2),
    PX(0,0,0,3,2,2,2,2),
    /* 4: crate top-left, palette 3 (or 4 on a goal) */
    PX(3,3,3,3,3,3,3,3),
    PX(3,1,1,1,1,1,1,1),
    PX(3,1,2,2,2,2,2,2),
    PX(3,1,2,3,3,3,3,3),
    PX(3,1,2,3,2,2,3,3),
    PX(3,1,2,3,1,2,2,3),
    PX(3,1,2,3,3,1,2,2),
    PX(3,1,2,3,3,3,1,2),
    /* 5: crate top-right */
    PX(3,3,3,3,3,3,3,3),
    PX(1,1,1,1,1,1,2,3),
    PX(2,2,2,2,2,2,3,3),
    PX(3,3,3,3,3,2,3,3),
    PX(3,3,3,3,3,2,3,3),
    PX(3,3,3,3,3,2,3,3),
    PX(3,3,3,3,3,2,3,3),
    PX(2,3,3,3,3,2,3,3),
    /* 6: crate bottom-left */
    PX(3,1,2,3,3,3,3,1),
    PX(3,1,2,3,3,3,3,3),
    PX(3,1,2,3,3,3,3,3),
    PX(3,1,2,3,3,3,3,3),
    PX(3,1,2,3,3,3,3,3),
    PX(3,1,2,2,2,2,2,2),
    PX(3,2,3,3,3,3,3,3),
    PX(3,3,3,3,3,3,3,3),
    /* 7: crate bottom-right */
    PX(2,2,3,3,3,2,3,3),
    PX(1,2,2,3,3,2,3,3),
    PX(3,1,2,2,3,2,3,3),
    PX(3,3,1,2,3,2,3,3),
    PX(3,3,3,3,3,2,3,3),
    PX(2,2,2,2,2,2,3,3),
    PX(3,3,3,3,3,3,3,3),
    PX(3,3,3,3,3,3,3,3),
};

const palette_color_t * const ts_sokoban_pals[] = {
    pal_wh_wall, pal_wh_floor, pal_wh_goal, pal_wh_crate, pal_wh_crate_ok, pal_wh_void
};
const tileset_def_t ts_sokoban = { ts_sokoban_tiles, 8, ts_sokoban_pals, 6 };
