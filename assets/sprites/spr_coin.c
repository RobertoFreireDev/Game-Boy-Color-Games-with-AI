#include "engine/engine.h"

/* 0 = transparent, 1 = dark gold outline, 2 = gold, 3 = shine */
const palette_color_t pal_coin[4] = { RGB8(0,0,0), RGB8(120,64,16), RGB8(248,192,40), RGB8(255,248,184) };

/* Spinning coin, 8x8, 4 frames: 0 face, 1 turning, 2 edge, 3 turning back */
const uint8_t spr_coin_tiles[] = {
    /* frame 0: face */
    PX(0,0,1,1,1,1,0,0),
    PX(0,1,2,2,2,2,1,0),
    PX(1,2,3,3,2,2,2,1),
    PX(1,2,3,2,2,2,2,1),
    PX(1,2,3,2,2,2,2,1),
    PX(1,2,2,2,2,2,2,1),
    PX(0,1,2,2,2,2,1,0),
    PX(0,0,1,1,1,1,0,0),
    /* frame 1: turning (shine on the left) */
    PX(0,0,0,1,1,0,0,0),
    PX(0,0,1,2,2,1,0,0),
    PX(0,0,1,3,2,1,0,0),
    PX(0,0,1,3,2,1,0,0),
    PX(0,0,1,3,2,1,0,0),
    PX(0,0,1,2,2,1,0,0),
    PX(0,0,1,2,2,1,0,0),
    PX(0,0,0,1,1,0,0,0),
    /* frame 2: edge */
    PX(0,0,0,1,1,0,0,0),
    PX(0,0,0,3,2,0,0,0),
    PX(0,0,0,3,2,0,0,0),
    PX(0,0,0,3,2,0,0,0),
    PX(0,0,0,3,2,0,0,0),
    PX(0,0,0,3,2,0,0,0),
    PX(0,0,0,3,2,0,0,0),
    PX(0,0,0,1,1,0,0,0),
    /* frame 3: turning back (shine on the right) */
    PX(0,0,0,1,1,0,0,0),
    PX(0,0,1,2,2,1,0,0),
    PX(0,0,1,2,3,1,0,0),
    PX(0,0,1,2,3,1,0,0),
    PX(0,0,1,2,3,1,0,0),
    PX(0,0,1,2,2,1,0,0),
    PX(0,0,1,2,2,1,0,0),
    PX(0,0,0,1,1,0,0,0),
};

const sprite_def_t spr_coin = { spr_coin_tiles, 1, 1, 4, pal_coin, 2 };
